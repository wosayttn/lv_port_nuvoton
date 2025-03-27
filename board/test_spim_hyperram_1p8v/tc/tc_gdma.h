#ifndef __TC_GDMA_H__
#define __TC_GDMA_H__

#include "dma350_lib.h"
#include "nu_bitutil.h"
#include "NuMicro.h"

typedef struct
{
#define DEF_CMDBUF_SIZE   16
    uint32_t m_cmdbuf[DEF_CMDBUF_SIZE];
} S_CMDBUF;

extern struct dma350_ch_dev_t *const GDMA_CH_DEV_S[];
extern int volatile g_bDone;

void tc_gdma_dsc_dump(S_CMDBUF *psCmdBufHead, int i32Length);

#endif /* __TC_GDMA_H__ */