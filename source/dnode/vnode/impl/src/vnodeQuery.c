/*
 * Copyright (c) 2019 TAOS Data, Inc. <jhtao@taosdata.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "vnodeQuery.h"
#include "vnodeDef.h"

static int32_t vnodeGetTableList(SVnode *pVnode, SRpcMsg *pMsg);

int vnodeQueryOpen(SVnode *pVnode) { return qWorkerInit(NULL, &pVnode->pQuery); }

int vnodeProcessQueryReq(SVnode *pVnode, SRpcMsg *pMsg, SRpcMsg **pRsp) {
  vInfo("query message is processed");
  return qWorkerProcessQueryMsg(pVnode, pVnode->pQuery, pMsg);
}

int vnodeProcessFetchReq(SVnode *pVnode, SRpcMsg *pMsg, SRpcMsg **pRsp) {
  vInfo("fetch message is processed");
  switch (pMsg->msgType) {
    case TDMT_VND_FETCH:
      return qWorkerProcessFetchMsg(pVnode, pVnode->pQuery, pMsg);
    case TDMT_VND_RES_READY:
      return qWorkerProcessReadyMsg(pVnode, pVnode->pQuery, pMsg);
    case TDMT_VND_TASKS_STATUS:
      return qWorkerProcessStatusMsg(pVnode, pVnode->pQuery, pMsg);
    case TDMT_VND_CANCEL_TASK:
      return qWorkerProcessCancelMsg(pVnode, pVnode->pQuery, pMsg);
    case TDMT_VND_DROP_TASK:
      return qWorkerProcessDropMsg(pVnode, pVnode->pQuery, pMsg);
    case TDMT_VND_SHOW_TABLES:
      return qWorkerProcessShowMsg(pVnode, pVnode->pQuery, pMsg);
    case TDMT_VND_SHOW_TABLES_FETCH:
      return vnodeGetTableList(pVnode, pMsg);
//      return qWorkerProcessShowFetchMsg(pVnode->pMeta, pVnode->pQuery, pMsg);
    default:
      vError("unknown msg type:%d in fetch queue", pMsg->msgType);
      return TSDB_CODE_VND_APP_ERROR;
  }
}

static int vnodeGetTableMeta(SVnode *pVnode, SRpcMsg *pMsg, SRpcMsg **pRsp) {
  STableInfoMsg * pReq = (STableInfoMsg *)(pMsg->pCont);
  STbCfg *        pTbCfg = NULL;
  STbCfg *        pStbCfg = NULL;
  tb_uid_t        uid;
  int32_t         nCols;
  int32_t         nTagCols;
  SSchemaWrapper *pSW;
  STableMetaMsg * pTbMetaMsg;
  SSchema *       pTagSchema;

  pTbCfg = metaGetTbInfoByName(pVnode->pMeta, pReq->tableFname, &uid);
  if (pTbCfg == NULL) {
    return -1;
  }

  if (pTbCfg->type == META_CHILD_TABLE) {
    pStbCfg = metaGetTbInfoByUid(pVnode->pMeta, pTbCfg->ctbCfg.suid);
    if (pStbCfg == NULL) {
      return -1;
    }

    pSW = metaGetTableSchema(pVnode->pMeta, pTbCfg->ctbCfg.suid, 0, true);
  } else {
    pSW = metaGetTableSchema(pVnode->pMeta, uid, 0, true);
  }

  nCols = pSW->nCols;
  if (pTbCfg->type == META_SUPER_TABLE) {
    nTagCols = pTbCfg->stbCfg.nTagCols;
    pTagSchema = pTbCfg->stbCfg.pTagSchema;
  } else if (pTbCfg->type == META_SUPER_TABLE) {
    nTagCols = pStbCfg->stbCfg.nTagCols;
    pTagSchema = pStbCfg->stbCfg.pTagSchema;
  } else {
    nTagCols = 0;
    pTagSchema = NULL;
  }

  pTbMetaMsg = (STableMetaMsg *)calloc(1, sizeof(STableMetaMsg) + sizeof(SSchema) * (nCols + nTagCols));
  if (pTbMetaMsg == NULL) {
    return -1;
  }

  strcpy(pTbMetaMsg->tbFname, pTbCfg->name);
  if (pTbCfg->type == META_CHILD_TABLE) {
    strcpy(pTbMetaMsg->stbFname, pStbCfg->name);
    pTbMetaMsg->suid = htobe64(pTbCfg->ctbCfg.suid);
  }
  pTbMetaMsg->numOfTags = htonl(nTagCols);
  pTbMetaMsg->numOfColumns = htonl(nCols);
  pTbMetaMsg->tableType = pTbCfg->type;
  pTbMetaMsg->tuid = htobe64(uid);
  pTbMetaMsg->vgId = htonl(pVnode->vgId);

  memcpy(pTbMetaMsg->pSchema, pSW->pSchema, sizeof(SSchema) * pSW->nCols);
  if (nTagCols) {
    memcpy(POINTER_SHIFT(pTbMetaMsg->pSchema, sizeof(SSchema) * pSW->nCols), pTagSchema, sizeof(SSchema) * nTagCols);
  }

  for (int i = 0; i < nCols + nTagCols; i++) {
    SSchema *pSch = pTbMetaMsg->pSchema + i;
    pSch->colId = htonl(pSch->colId);
    pSch->bytes = htonl(pSch->bytes);
  }

  return 0;
}

/**
 * @param pVnode
 * @param pMsg
 * @param pRsp
 */
static int32_t vnodeGetTableList(SVnode *pVnode, SRpcMsg *pMsg) {
  SMTbCursor* pCur = metaOpenTbCursor(pVnode->pMeta);
  SArray* pArray = taosArrayInit(10, POINTER_BYTES);

  char* name = NULL;
  int32_t totalLen = 0;
  while ((name = metaTbCursorNext(pCur)) != NULL) {
    taosArrayPush(pArray, &name);
    totalLen += strlen(name);
  }

  metaCloseTbCursor(pCur);

  int32_t rowLen = (TSDB_TABLE_NAME_LEN + VARSTR_HEADER_SIZE) + 8 + 2 + (TSDB_TABLE_NAME_LEN + VARSTR_HEADER_SIZE) + 8 + 4;
  int32_t numOfTables = (int32_t) taosArrayGetSize(pArray);

  int32_t payloadLen = rowLen * numOfTables;
//  SVShowTablesFetchReq *pFetchReq = pMsg->pCont;

  SVShowTablesFetchRsp *pFetchRsp = (SVShowTablesFetchRsp *)rpcMallocCont(sizeof(SVShowTablesFetchRsp) + payloadLen);
  memset(pFetchRsp, 0, sizeof(struct SVShowTablesFetchRsp) + payloadLen);

  char* p = pFetchRsp->data;
  for(int32_t i = 0; i < numOfTables; ++i) {
    char* n = taosArrayGetP(pArray, i);
    STR_TO_VARSTR(p, n);

    p += (TSDB_TABLE_NAME_LEN + VARSTR_HEADER_SIZE);
  }

  pFetchRsp->numOfRows = htonl(numOfTables);
  pFetchRsp->precision = 0;

  SRpcMsg rpcMsg = {
      .handle  = pMsg->handle,
      .ahandle = pMsg->ahandle,
      .pCont   = pFetchRsp,
      .contLen = sizeof(SVShowTablesFetchRsp) + payloadLen,
      .code    = 0,
  };

  rpcSendResponse(&rpcMsg);
  return 0;
}