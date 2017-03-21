/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include <memory.h> /* for memset */
#include "conecta4.h"

/* Default timeout can be changed using clnt_control() */
static struct timeval TIMEOUT = { 25, 0 };

int *
registerplayer_1(tMessage *argp, CLIENT *clnt)
{
	static int clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, registerPlayer,
		(xdrproc_t) xdr_tMessage, (caddr_t) argp,
		(xdrproc_t) xdr_int, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

tBlock *
getgamestatus_1(tMessage *argp, CLIENT *clnt)
{
	static tBlock clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, getGameStatus,
		(xdrproc_t) xdr_tMessage, (caddr_t) argp,
		(xdrproc_t) xdr_tBlock, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

tBlock *
insertchipinboard_1(tColumn *argp, CLIENT *clnt)
{
	static tBlock clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, insertChipInBoard,
		(xdrproc_t) xdr_tColumn, (caddr_t) argp,
		(xdrproc_t) xdr_tBlock, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}
