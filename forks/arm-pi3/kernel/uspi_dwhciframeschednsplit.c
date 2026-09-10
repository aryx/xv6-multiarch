//
// dwhciframeschednsplit.c
//
// USPi - An USB driver for Raspberry Pi written in C
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include <uspi/dwhciframeschednsplit.h>
#include <uspi/dwhciregister.h>
#include <uspi/assert.h>

#define FRAME_UNSET	(DWHCI_MAX_FRAME_NUMBER+1)

void DWHCIFrameSchedulerNoSplit (TDWHCIFrameSchedulerNoSplit *pThis, boolean bIsPeriodic)
{
	assert (pThis != 0);

	TDWHCIFrameScheduler *pBase = (TDWHCIFrameScheduler *) pThis;

	pBase->_DWHCIFrameScheduler = _DWHCIFrameSchedulerNoSplit;
	pBase->StartSplit = DWHCIFrameSchedulerNoSplitStartSplit;
	pBase->CompleteSplit = DWHCIFrameSchedulerNoSplitCompleteSplit;
	pBase->TransactionComplete = DWHCIFrameSchedulerNoSplitTransactionComplete;
	pBase->WaitForFrame = DWHCIFrameSchedulerNoSplitWaitForFrame;
	pBase->IsOddFrame = DWHCIFrameSchedulerNoSplitIsOddFrame;

	pThis->m_bIsPeriodic = bIsPeriodic;
	pThis->m_nNextFrame = FRAME_UNSET;

}

void _DWHCIFrameSchedulerNoSplit (TDWHCIFrameScheduler *pBase)
{
}

void DWHCIFrameSchedulerNoSplitStartSplit (TDWHCIFrameScheduler *pBase)
{
	assert (0);
}

boolean DWHCIFrameSchedulerNoSplitCompleteSplit (TDWHCIFrameScheduler *pBase)
{
	assert (0);
	return FALSE;
}

void DWHCIFrameSchedulerNoSplitTransactionComplete (TDWHCIFrameScheduler *pBase, u32 nStatus)
{
	assert (0);
}

void DWHCIFrameSchedulerNoSplitWaitForFrame (TDWHCIFrameScheduler *pBase)
{
	TDWHCIFrameSchedulerNoSplit *pThis = (TDWHCIFrameSchedulerNoSplit *) pBase;
	assert (pThis != 0);

	TDWHCIRegister FrameNumber;
	DWHCIRegister (&FrameNumber, DWHCI_HOST_FRM_NUM);

	pThis->m_nNextFrame = (DWHCI_HOST_FRM_NUM_NUMBER (DWHCIRegisterRead (&FrameNumber))+1) & DWHCI_MAX_FRAME_NUMBER;

	if (!pThis->m_bIsPeriodic)
	{
		/* claude: this was an exact-equality wait -
		 *
		 *   while ((FRM_NUM(read) & MAX) != m_nNextFrame) ;
		 *
		 * - which assumes the host frame counter is observed at EVERY
		 * value it passes through. That holds on real silicon polled by
		 * a CPU running at bus speed; it does NOT hold under QEMU,
		 * where the counter advances from a timer while TCG executes
		 * the spin loop in blocks, so the one value being waited for
		 * can be stepped straight over. Miss it and the wait becomes
		 * a full 14-bit wraparound - 0x4000 frames, ~16s - and can miss
		 * again, which is exactly how USB enumeration hung here: the
		 * loop was measured spinning 3.4 MILLION times for a single
		 * frame, then never matching at all on a later transfer.
		 *
		 * Same class of bug, and same fix, as forks/arm-pi2's own Bug 1
		 * and forks/arm-pi1's own Bug 4 (both exact-equality busy-waits
		 * on a free-running counter). Wait for "has reached or passed
		 * m_nNextFrame" instead, computed as a wraparound-safe modular
		 * difference: while the counter is still BEHIND the target the
		 * difference lands in the upper half of the 14-bit space, and
		 * anywhere at-or-past it lands in the lower half.
		 *
		 * Correct on real hardware too, and strictly better there: it
		 * exits at exactly the same frame the old code did when the
		 * frame is observed, and no longer stalls for 16 seconds when a
		 * slow poll happens to miss it. Only the NoSplit scheduler is
		 * touched, and only its non-periodic branch - a plain pacing
		 * delay to the next frame boundary. The equivalent loop in
		 * uspi_dwhciframeschedper.c is deliberately left alone: it runs
		 * only on the split path (real hardware, devices behind the
		 * board's built-in hub), and there landing on one SPECIFIC
		 * microframe is a real scheduling requirement, not just pacing.
		 */
		while (((  (DWHCI_HOST_FRM_NUM_NUMBER (DWHCIRegisterRead (&FrameNumber))
			   & DWHCI_MAX_FRAME_NUMBER)
			 - pThis->m_nNextFrame) & DWHCI_MAX_FRAME_NUMBER)
		       > (DWHCI_MAX_FRAME_NUMBER / 2))
		{
			// do nothing
		}
	}

	_DWHCIRegister (&FrameNumber);
}

boolean DWHCIFrameSchedulerNoSplitIsOddFrame (TDWHCIFrameScheduler *pBase)
{
	TDWHCIFrameSchedulerNoSplit *pThis = (TDWHCIFrameSchedulerNoSplit *) pBase;
	assert (pThis != 0);

	return pThis->m_nNextFrame & 1 ? TRUE : FALSE;
}
