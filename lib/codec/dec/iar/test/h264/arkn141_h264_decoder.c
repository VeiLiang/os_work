/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2006 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Abstract : H264 Encoder testbench for linux
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
/* For printing and file IO */
#include <stdio.h>

/* For dynamic memory allocation */
#include <stdlib.h>

/* For memset, strcpy and strlen */
#include <string.h>

#include "dwl.h"
/* For Hantro H.264 encoder */
#include "h264decapi.h"
#include "h264hwd_container.h"
#include "regdrv.h"

#include "arkn141_h264_decoder.h"

static const u32 clock_gating = DEC_X170_INTERNAL_CLOCK_GATING;
static const u32 data_discard = DEC_X170_DATA_DISCARD_ENABLE;
static const u32 latency_comp = DEC_X170_LATENCY_COMPENSATION;
static const u32 output_picture_endian = DEC_X170_OUTPUT_PICTURE_ENDIAN;
static const u32 bus_burst_length = DEC_X170_BUS_BURST_LENGTH;
static const u32 asic_service_priority = DEC_X170_ASIC_SERVICE_PRIORITY;
static const u32 output_format = DEC_X170_OUTPUT_FORMAT;


int h264_arkn141_config_decoder (h264_arkn141_H264DecInst pDec)
{
	H264DecInst decInst = pDec;

	if(decInst == NULL)
		return -1;

    SetDecRegister(((decContainer_t *) decInst)->h264Regs, HWIF_DEC_LATENCY,
                   latency_comp);
    SetDecRegister(((decContainer_t *) decInst)->h264Regs, HWIF_DEC_CLK_GATE_E,
                   clock_gating);
    SetDecRegister(((decContainer_t *) decInst)->h264Regs, HWIF_DEC_OUT_TILED_E,
                   output_format);
    SetDecRegister(((decContainer_t *) decInst)->h264Regs, HWIF_DEC_OUT_ENDIAN,
                   output_picture_endian);
    SetDecRegister(((decContainer_t *) decInst)->h264Regs, HWIF_DEC_MAX_BURST,
                   bus_burst_length);
    if ((DWLReadAsicID() >> 16) == 0x8170U)
    {
        SetDecRegister(((decContainer_t *) decInst)->h264Regs, HWIF_PRIORITY_MODE,
                       asic_service_priority);
    }
    SetDecRegister(((decContainer_t *) decInst)->h264Regs, HWIF_DEC_DATA_DISC_E,
                   data_discard);

	 return 0;
}



