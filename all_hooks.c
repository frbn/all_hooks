#include "postgres.h"
#include "libpq/auth.h"
// fmgr_hook
#include "fmgr.h"
#include "utils/elog.h"
// planner_hook
#include "optimizer/planner.h"
// ProcessUtility_hook
#include "postgres.h"
#include "tcop/utility.h"

// Executor
// executeCheckPerms_hook
#include "executor/executor.h"
// ExecutorStart_hook
// ExecutorRun_hook
// ExecutorFinish_hook
// ExecutorEnd_hook

// ExecutorEnd_hook
#include <access/heapam.h>
#include <catalog/namespace.h>
#include <miscadmin.h>
#include <storage/bufmgr.h>
#include <utils/builtins.h>
#include <utils/guc.h>
#include <optimizer/optimizer.h>
#include <lib/ilist.h>

// PLPGSQL
#include "plpgsql.h"

// emit_log_hook
#include "utils/elog.h"

//check_password_hook
#include "commands/user.h"

//shmem_startup
#include "storage/shmem.h"
#include "storage/ipc.h"

// ----------

// ExecutorEnd_hook
static ExecutorEnd_hook_type ah_original_ExecutorEnd_hook = NULL;
void ah_ExecutorEnd_hook(QueryDesc *q);

// fmgr_hook
static fmgr_hook_type ah_original_fmgr_hook = NULL;
void ah_fmgr_hook(FmgrHookEventType event, FmgrInfo * flinfo, Datum *arg) ;

// needs_fmgr_hook
static needs_fmgr_hook_type ah_original_needs_fmgr_hook = NULL;
bool ah_needs_fmgr_hook (Oid fn_oid);


// PLPGSQL
static PLpgSQL_plugin  *ah_original_plpgsql_plugin = NULL;

static void ah_plpgsql_func_init_hook(PLpgSQL_execstate *estate, PLpgSQL_function *func);
static void ah_plpgsql_func_beg_hook(PLpgSQL_execstate *estate, PLpgSQL_function *func);
static void ah_plpgsql_func_end_hook(PLpgSQL_execstate *estate, PLpgSQL_function *func);

static void ah_plpgsql_stmt_beg_hook(PLpgSQL_execstate * estate, PLpgSQL_stmt* stmt) ;
static void ah_plpgsql_stmt_end_hook(PLpgSQL_execstate * estate, PLpgSQL_stmt* stmt) ;

// PLPGSQL
static PLpgSQL_plugin	ah_plugin_funcs = {
	ah_plpgsql_func_init_hook,
	ah_plpgsql_func_beg_hook,
	ah_plpgsql_func_end_hook,
	ah_plpgsql_stmt_beg_hook,
	ah_plpgsql_stmt_end_hook,
	NULL,
	NULL
};

// emit_log_hook
static emit_log_hook_type ah_original_emit_log_hook = NULL;
static void ah_emit_log_hook(ErrorData* );
// antirecursion 
static bool ah_emit_log_hook_in_hook = false;

//check_password_hook
static check_password_hook_type ah_original_check_password_hook = NULL;
void ah_check_password_hook(const char *username, const char *shadow_pass, PasswordType password_type, Datum validuntil_time, bool validuntil_null);

// ClientAuthentication_hook
static ClientAuthentication_hook_type ah_original_client_authentication_hook = NULL;
static void ah_ClientAuthentication_hook(Port * port, int status);

//shmem_startup
static shmem_startup_hook_type ah_original_shmem_startup_hook = NULL;
void ah_shmem_startup_hook(void);


// Executor

// executeCheckPerms_hook
static ExecutorCheckPerms_hook_type ah_original_ExecutorCheckPerms_hook = NULL;

// ExecutorStart_hook
static ExecutorStart_hook_type ah_original_ExecutorStart_hook = NULL;

// ExecutorStart_hook
void ah_ExecutorStart_hook (QueryDesc *queryDesc, int eflags);

// ExecutorRun_hook
static ExecutorRun_hook_type ah_original_ExecutorRun_hook = NULL;
// ExecutorFinish_hook
static ExecutorFinish_hook_type ah_original_ExecutorFinish_hook = NULL;
void ah_ExecutorFinish_hook(QueryDesc *queryDesc);
// ExecutorRun_hook
void ah_ExecutorRun_hook
(
	QueryDesc *queryDesc,
	ScanDirection direction,
	uint64 count,
	bool execute_once
);

// planner_hook
static planner_hook_type ah_original_planner_hook  = NULL;

// ProcessUtility_hook
static ProcessUtility_hook_type ah_original_ProcessUtility_hook = NULL;
void ah_ProcessUtility_hook(
	PlannedStmt *pstmt,
	const char *queryString,
	bool readOnlyTree,
	ProcessUtilityContext context,
	ParamListInfo params,
	QueryEnvironment *queryEnv,
	DestReceiver *dest,
	QueryCompletion *completionTag);

// Executor
// executeCheckPerms_hook
static bool ah_ExecutorCheckPerms_hook (List*, List* , bool );

// ----------------------------------------
// ----------------------------------------
// FUNCTIONS

// planner_hook
static PlannedStmt *
ah_planner_hook(Query *parse, const char *query_st, int cursorOptions, ParamListInfo boundp) 
{
  PlannedStmt *result;

  elog(WARNING, "planner hook called");

  if (ah_original_planner_hook){
		result = ah_original_planner_hook(parse,query_st,cursorOptions, boundp);
	}
	else
	{
		result = standard_planner(parse, query_st, cursorOptions, boundp);
	}
	return result;
}

// ProcessUtility_hook
void ah_ProcessUtility_hook(
	PlannedStmt *pstmt, 
	const char *queryString,
	bool readOnlyTree,
	ProcessUtilityContext context,
	ParamListInfo params,
	QueryEnvironment *queryEnv,
	DestReceiver *dest,
	QueryCompletion *completionTag)
{
    elog(WARNING,"ProcessUtility hook called");
	if (ah_original_ProcessUtility_hook)
	{
		ah_original_ProcessUtility_hook(pstmt, queryString,readOnlyTree,context,params,queryEnv,dest, completionTag);
    }
	else
	{
		standard_ProcessUtility(pstmt,queryString, readOnlyTree, context, params, queryEnv, dest, completionTag);
	}
}

// Executor
// executeCheckPerms_hook
static bool ah_ExecutorCheckPerms_hook (List* tableList, List* rteperminfos, bool abort)
{

	elog(WARNING, "ExecutorCheckPerms_hook called");

	return true;
}

// ExecutorStart_hook
void ah_ExecutorStart_hook (QueryDesc *queryDesc, int eflags)
{

	elog(DEBUG1, "ExecutorStart_hook called");

	if (ah_original_ExecutorStart_hook)
	{
		ah_original_ExecutorStart_hook(queryDesc, eflags);
	}
	else
	{
		standard_ExecutorStart(queryDesc, eflags);
	}
}

// ExecutorRun_hook
void ah_ExecutorRun_hook(
	QueryDesc *queryDesc,
	ScanDirection direction,
	uint64 count, 
	bool execute_once)
{

    elog(WARNING, "ExecutorRun_hook called");

	if (ah_original_ExecutorRun_hook){
    	ah_original_ExecutorRun_hook(queryDesc, direction, count, execute_once);

	}
	else
	{
    	standard_ExecutorRun(queryDesc, direction, count, execute_once);
  	}
}

// ExecutorFinish_hook
void ah_ExecutorFinish_hook(QueryDesc *queryDesc)
{

  elog(WARNING, "ExecutorFinish_hook called");
    if (ah_original_ExecutorFinish_hook)
	{
    ah_original_ExecutorFinish_hook(queryDesc);
    }
	else
	{
      standard_ExecutorFinish(queryDesc);
	}
}

// ExecutorEnd_hook
void ah_ExecutorEnd_hook(QueryDesc *q)
{
	elog(WARNING,"ExecutorEnd hook called");
	if (ah_original_ExecutorEnd_hook)
    	ah_original_ExecutorEnd_hook(q);
  	else
    	standard_ExecutorEnd(q);
}

// fmgr_hook
void ah_fmgr_hook(FmgrHookEventType event, FmgrInfo * flinfo, Datum *arg){

  elog(WARNING,"fmgr hook called");
  if (ah_original_fmgr_hook)
    ah_original_fmgr_hook(event,flinfo,arg);
}

// needs_fmgr_hook
bool ah_needs_fmgr_hook (Oid fn_oid)
{
	elog(WARNING, "needs_fmgr_hook_type called");
	if (ah_original_needs_fmgr_hook)
	{
	  elog(WARNING, "ah_original_needs_fmgr_hook called");
		return ah_original_needs_fmgr_hook(fn_oid);
	}else
	{
		return true;
	}
}
 
// PLPGSQL
static void ah_plpgsql_stmt_beg_hook(PLpgSQL_execstate * estate, PLpgSQL_stmt* stmt)
{
  elog(WARNING,"stmt_beg hook called");
}

static void ah_plpgsql_stmt_end_hook(PLpgSQL_execstate * estate, PLpgSQL_stmt* stmt)
{
  elog(WARNING,"stmt_end hook called");
}

static void ah_plpgsql_func_init_hook(PLpgSQL_execstate *estate, PLpgSQL_function *func)

{
  elog(WARNING,"func_init hook called");
}
static void ah_plpgsql_func_beg_hook(PLpgSQL_execstate *estate, PLpgSQL_function *func)
{
  elog(WARNING,"func_beg hook called");
}
static void ah_plpgsql_func_end_hook(PLpgSQL_execstate *estate, PLpgSQL_function *func)
{
  elog(WARNING,"func_end hook called");

}

// emit_log_hook
void ah_emit_log_hook(ErrorData * eData)
{

	if (! ah_emit_log_hook_in_hook)
	{
		ah_emit_log_hook_in_hook = true;
		elog(WARNING, "ah_emit_log_hook called");
	}

	if (ah_original_emit_log_hook)
	{
		ah_original_emit_log_hook(eData);
	}


}

//check_password_hook
void ah_check_password_hook(const char *username, const char *shadow_pass, PasswordType password_type, Datum validuntil_time, bool validuntil_null)
{

  elog(WARNING,"check_password_hook called");

	if (ah_original_check_password_hook)
	{
		ah_original_check_password_hook(username, shadow_pass, password_type, validuntil_time, validuntil_null);
	}
}

void ah_ClientAuthentication_hook(Port * port, int status)
{

   // If any other extension registered its own hook handler,
    // call it before performing our own logic.
    if (ah_original_client_authentication_hook)
        ah_original_client_authentication_hook(port, status);

    if (status != STATUS_OK)
    {
      elog(WARNING,"ah_ClientAuthentication_hook status KO");
    }else{
      elog(WARNING,"ah_ClientAuthentication_hook called OK");
    }
}

//shmem_startup
void ah_shmem_startup_hook(void)
{

	if (ah_original_shmem_startup_hook){
		ah_original_shmem_startup_hook();
	}
	elog(WARNING,"shmem_startup_hook called");

}

PG_MODULE_MAGIC;

/* Function definitions */

void _PG_init(void);
void _PG_fini(void);



// Called upon extension load.
void _PG_init(void)
{
	PLpgSQL_plugin **plugin_ptr;

	elog(WARNING, "all_hooks init");

	elog(WARNING,"hooking: plpgsql");
	/* Link us into the PL/pgSQL executor. */
	plugin_ptr = (PLpgSQL_plugin **)find_rendezvous_variable("PLpgSQL_plugin");
	ah_original_plpgsql_plugin = *plugin_ptr;
	*plugin_ptr = &ah_plugin_funcs;


  // shmem_startup_hook
  elog(WARNING,"hooking: shmem_startup_hook");
  ah_original_shmem_startup_hook = shmem_startup_hook;
  shmem_startup_hook = ah_shmem_startup_hook;

  // planner_hook
  elog(WARNING,"hooking: planner_hook");
  ah_original_planner_hook = planner_hook;
  planner_hook = ah_planner_hook;

  // ProcessUtility_hook
  elog(WARNING,"hooking: ProcessUtility_hook");
  ah_original_ProcessUtility_hook = ProcessUtility_hook;
  ProcessUtility_hook = ah_ProcessUtility_hook;

  // ExecutorStart_hook
  elog(WARNING,"hooking: ExecutorStart_hook");
  ah_original_ExecutorStart_hook = ExecutorStart_hook;
  ExecutorStart_hook = ah_ExecutorStart_hook;

  //ExecutorRun_hook
  elog(WARNING,"hooking: ExecutorRun_hook");
  ah_original_ExecutorRun_hook = ExecutorRun_hook;
  ExecutorRun_hook = ah_ExecutorRun_hook;

  //ExecutorEnd_hook
  elog(WARNING,"hooking: ExecutorEnd_hook");
  ah_original_ExecutorEnd_hook = ExecutorEnd_hook;
  ExecutorEnd_hook = ah_ExecutorEnd_hook;

  // ExecutorCheckPerms_hook
  elog(WARNING,"hooking: ExecutorCheckPerms_hook");
  ah_original_ExecutorCheckPerms_hook = ExecutorCheckPerms_hook;
  ExecutorCheckPerms_hook = ah_ExecutorCheckPerms_hook;

  //ExecutorFinish_hook_type
  elog(WARNING,"hooking: ExecutorFinish_hook");
  ah_original_ExecutorFinish_hook = ExecutorFinish_hook;
  ExecutorFinish_hook = ah_ExecutorFinish_hook;

  // needs_fmgr_hook
  elog(WARNING,"hooking: needs_fmgr_hook");
  ah_original_needs_fmgr_hook = needs_fmgr_hook;
  needs_fmgr_hook = ah_needs_fmgr_hook;

  // fmgr_hook
  elog(WARNING,"hooking: fmgr_hook");
  ah_original_fmgr_hook = fmgr_hook;
  fmgr_hook = ah_fmgr_hook;

  // check_password_hook
  elog(WARNING,"hooking: check_password_hook");
  ah_original_check_password_hook = check_password_hook;
  check_password_hook = ah_check_password_hook;

  // ClientAuthentication_hook
  elog(WARNING,"hooking: ClientAuthentication_hook");
  ah_original_client_authentication_hook = ClientAuthentication_hook;
  ClientAuthentication_hook = ah_ClientAuthentication_hook;

  // emit_log_hook
  elog(WARNING,"hooking: emit_log_hook");
  ah_original_emit_log_hook = emit_log_hook;
  emit_log_hook = ah_emit_log_hook;

}

// Called with extension unload.
void _PG_fini(void)
{
    // Return back the original hook value.

	PLpgSQL_plugin **plugin_ptr;
	plugin_ptr = (PLpgSQL_plugin **)find_rendezvous_variable("PLpgSQL_plugin");
	*plugin_ptr = ah_original_plpgsql_plugin;
	ah_original_plpgsql_plugin = NULL;

    ClientAuthentication_hook = ah_original_client_authentication_hook;
    ExecutorEnd_hook = ah_original_ExecutorEnd_hook;
    planner_hook = ah_original_planner_hook;
    ProcessUtility_hook = ah_original_ProcessUtility_hook;
    ExecutorStart_hook = ah_original_ExecutorStart_hook;
    ExecutorRun_hook = ah_original_ExecutorRun_hook;
    ExecutorEnd_hook = ah_original_ExecutorEnd_hook;
    ExecutorFinish_hook = ah_original_ExecutorFinish_hook;
    ExecutorCheckPerms_hook = ah_original_ExecutorCheckPerms_hook;
    emit_log_hook = ah_original_emit_log_hook;
    fmgr_hook = ah_original_fmgr_hook;
    check_password_hook = ah_original_check_password_hook;
    shmem_startup_hook = ah_original_shmem_startup_hook;
}
