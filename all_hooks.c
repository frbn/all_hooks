/*-------------------------------------------------------------------------
 *
 * all_hooks
 *
 *
 * Copyright (c) 2025, Franck Boudehen, Dalibo
 *
 *-------------------------------------------------------------------------
 */
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

// check_password_hook
#include "commands/user.h"

// shmem_startup
#include "storage/shmem.h"
#include "storage/ipc.h"

// explain_hooks
#if PG_VERSION_NUM >= 180000
#include "commands/explain.h"
#include "commands/explain_format.h"
#include "commands/explain_state.h"

// set_rel_pathlist_hook
#include "optimizer/paths.h"

#endif

// ObjectAccess
#include "catalog/objectaccess.h"

// explain_get_index_name
#include "commands/explain.h"
#include "utils/lsyscache.h"

// explain_validate_options
#if PG_VERSION_NUM >= 180000
#include "commands/explain_state.h"
#endif

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

static void ah_plpgsql_func_setup_hook(PLpgSQL_execstate *estate, PLpgSQL_function *func);
static void ah_plpgsql_func_beg_hook(PLpgSQL_execstate *estate, PLpgSQL_function *func);
static void ah_plpgsql_func_end_hook(PLpgSQL_execstate *estate, PLpgSQL_function *func);

static void ah_plpgsql_stmt_beg_hook(PLpgSQL_execstate * estate, PLpgSQL_stmt* stmt) ;
static void ah_plpgsql_stmt_end_hook(PLpgSQL_execstate * estate, PLpgSQL_stmt* stmt) ;

// PLPGSQL
static PLpgSQL_plugin	ah_plugin_funcs =
{
	ah_plpgsql_func_setup_hook,
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

// explain_hooks
#if PG_VERSION_NUM >= 180000
static explain_per_node_hook_type ah_original_explain_per_node_hook = NULL;
static explain_per_plan_hook_type ah_original_explain_per_plan_hook = NULL;
static void ah_explain_per_node_hook( PlanState *planstate, List *ancestors,
	const char *relationship, const char *plan_name, ExplainState *es);
static void ah_explain_per_plan_hook( PlannedStmt *plannedstmt, IntoClause *into,
	ExplainState *es, const char *queryString, ParamListInfo params,
	QueryEnvironment *queryEnv);
#endif

// Executor

// executeCheckPerms_hook
static ExecutorCheckPerms_hook_type ah_original_ExecutorCheckPerms_hook;

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
void ah_ExecutorRun_hook(
	QueryDesc *queryDesc,
	ScanDirection direction,
	uint64 count
#if PG_VERSION_NUM < 180000
	,bool execute_once
#endif
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
#if PG_VERSION_NUM <160000
bool ah_ExecutorCheckPerms_hook (List* tableList, bool abort);
#else
bool ah_ExecutorCheckPerms_hook (List* tableList, List* rteperminfos, bool abort);
#endif

//paths_list
#if PG_VERSION_NUM >=180000
set_rel_pathlist_hook_type ah_original_set_rel_pathlist_hook;
static void ah_set_rel_pathlist_hook(PlannerInfo *root, RelOptInfo *rel,
		Index rti, RangeTblEntry *rte);

#endif

// ObjectAccess
object_access_hook_type ah_original_object_access_hook ;
static void ah_object_access_hook(ObjectAccessType access,Oid classId, Oid objectId,int subId,void *arg);

object_access_hook_type_str ah_original_object_access_hook_str ;
static void ah_object_access_hook_str(ObjectAccessType access, Oid classId,const char *objectStr,int subId,void *arg);

// explain_get_index_name_hook
explain_get_index_name_hook_type ah_original_explain_get_index_name_hook;
static const char * ah_explain_get_index_name_hook(Oid indexId);

#if PG_VERSION_NUM >= 180000
// explain_validate_options_hook
explain_validate_options_hook_type ah_original_explain_validate_option_hook;
static void ah_explain_validate_options_hook (struct ExplainState *es,
											  	List *options,
												ParseState *pstate);
#endif

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
#if PG_VERSION_NUM <160000
bool ah_ExecutorCheckPerms_hook (List* tableList, bool abort)
#else
bool ah_ExecutorCheckPerms_hook (List* tableList, List* rteperminfos, bool abort)
#endif
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
	uint64 count
#if PG_VERSION_NUM < 180000
	, bool execute_once
#endif
)
{

	elog(WARNING, "ExecutorRun_hook called");

	if (ah_original_ExecutorRun_hook)
	{
#if PG_VERSION_NUM < 180000
		ah_original_ExecutorRun_hook(queryDesc, direction, count, execute_once);
#else
		ah_original_ExecutorRun_hook(queryDesc, direction, count);
#endif

	}
	else
	{
#if PG_VERSION_NUM < 180000
		standard_ExecutorRun(queryDesc, direction, count, execute_once);
#else
		standard_ExecutorRun(queryDesc, direction, count);
#endif
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
	if (ah_original_plpgsql_plugin)
	{
		if (ah_original_plpgsql_plugin->stmt_beg)
		{
			ah_original_plpgsql_plugin->stmt_beg(estate, stmt);
		}

	}
}

static void ah_plpgsql_stmt_end_hook(PLpgSQL_execstate * estate, PLpgSQL_stmt* stmt)
{
	elog(WARNING,"stmt_end hook called");
	if (ah_original_plpgsql_plugin)
	{
		if (ah_original_plpgsql_plugin->stmt_end)
		{
			ah_original_plpgsql_plugin->stmt_end(estate, stmt);
		}

	}
}

static void ah_plpgsql_func_setup_hook(PLpgSQL_execstate *estate, PLpgSQL_function *func)
{
	elog(WARNING,"func_setup hook called");
	if (ah_original_plpgsql_plugin)
	{
		if (ah_original_plpgsql_plugin->func_setup)
		{
			ah_original_plpgsql_plugin->func_setup(estate, func);
		}
	}
}

static void ah_plpgsql_func_beg_hook(PLpgSQL_execstate *estate, PLpgSQL_function *func)
{
	elog(WARNING,"func_beg hook called");
	if (ah_original_plpgsql_plugin)
	{
		if (ah_original_plpgsql_plugin->func_beg)
		{
			ah_original_plpgsql_plugin->func_beg(estate, func);
		}

	}
}

static void ah_plpgsql_func_end_hook(PLpgSQL_execstate *estate, PLpgSQL_function *func)
{
	elog(WARNING,"func_end hook called");
	if (ah_original_plpgsql_plugin)
	{
		if (ah_original_plpgsql_plugin->func_end)
		{
			ah_original_plpgsql_plugin->func_end(estate, func);
		}

	}

}

// emit_log_hook
void ah_emit_log_hook(ErrorData * eData)
{

	// we avoid log looping
	if (! ah_emit_log_hook_in_hook)
	{
		ah_emit_log_hook_in_hook = true;
		elog(WARNING, "emit_log_hook called");
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

	if (ah_original_shmem_startup_hook)
	{
		ah_original_shmem_startup_hook();
	}
	elog(WARNING,"shmem_startup_hook called");

}


#if PG_VERSION_NUM >= 180000
static void ah_explain_per_node_hook(PlanState *planstate, List *ancestors,
									const char *relationship,
									const char *plan_name,
									ExplainState *es)
{
	elog(WARNING,"explain_per_node_hook called");
	if (ah_original_explain_per_node_hook)
	{
		ah_original_explain_per_node_hook(planstate, ancestors, relationship, plan_name, es);
	}

}

static void ah_explain_per_plan_hook(PlannedStmt *plannedstmt,
									 IntoClause *into,
									 ExplainState *es,
									 const char *queryString,
									 ParamListInfo params,
QueryEnvironment *queryEnv)
{
	if (ah_original_explain_per_plan_hook)
		ah_original_explain_per_plan_hook(plannedstmt, into, es, queryString, params, queryEnv);
	elog(WARNING,"explain_per_plan_hook called");

}
#endif

#if PG_VERSION_NUM >= 180000
	/*
	 * Allow to editorialize on the set of Paths for this base
	 * relation.  It could add new paths (such as CustomPaths) by calling
	 * add_path(), or add_partial_path() if parallel aware.  It could also
	 * delete or modify paths added by the core code.
	 */

static void ah_set_rel_pathlist_hook(PlannerInfo *root, RelOptInfo *rel,
		Index rti, RangeTblEntry *rte){
	char rtekind_str[16];

	switch (rte->rtekind)
	{
		case RTE_RELATION : strcpy(rtekind_str,"RELATION");
		break;
		case RTE_SUBQUERY : strcpy(rtekind_str,"SUBQUERY");
		break;
		case RTE_JOIN : strcpy(rtekind_str,"JOIN");
		break;
		case RTE_FUNCTION : strcpy(rtekind_str,"FUNCTION");
		break;
		case RTE_TABLEFUNC : strcpy(rtekind_str,"TABLEFUNC");
		break;
		case RTE_VALUES : strcpy(rtekind_str,"VALUES");
		break;
		case RTE_CTE : strcpy(rtekind_str,"CTE");
		break;
		case RTE_NAMEDTUPLESTORE : strcpy(rtekind_str,"NAMEDTUPLESTORE");
		break;
		case RTE_RESULT : strcpy(rtekind_str,"RESULT");
		break;
		case RTE_GROUP : strcpy(rtekind_str,"GROUP");
		break;

		default : strcpy(rtekind_str,"unknown");
	}

		elog(WARNING,"set_rel_pathlist_hook called: %s",rtekind_str);

	// preserve hooks chaining
	if (ah_original_set_rel_pathlist_hook){
		ah_original_set_rel_pathlist_hook(root,rel,rti,rte);
	}

}
#endif

// objectaccess

static void ah_object_access_hook(ObjectAccessType access,Oid classId, Oid objectId,int subId,void *arg)
{
	char * accessName;
	switch (access) {
		case OAT_POST_CREATE : 	accessName= "OAT_POST_CREATE ";
		break;
		case OAT_DROP :accessName= "OAT_DROP ";
		break;
		case OAT_POST_ALTER :accessName= "OAT_POST_ALTER ";
		break;
		case OAT_NAMESPACE_SEARCH :accessName= "OAT_NAMESPACE_SEARCH ";
		break;
		case OAT_FUNCTION_EXECUTE :accessName= "OAT_FUNCTION_EXECUTE ";
		break;
		case OAT_TRUNCATE :accessName= "OAT_TRUNCATE ";
		break;
		default : accessName= "unknown";

	}
	elog(WARNING, "object_access_hook called: class %u / object %u / %s", classId,objectId, accessName);

	if (ah_original_object_access_hook)
	{
		ah_original_object_access_hook(access,classId,objectId,subId,arg);
	}
}

// objectaccessStr

static void ah_object_access_hook_str(ObjectAccessType access, Oid classId,const char *objectStr,int subId,void *arg)
{
	elog(WARNING, "object_access_hook_str called");
	if (ah_original_object_access_hook_str)
	{
		ah_original_object_access_hook_str(access, classId,objectStr,subId,arg);
	}

}

// explain_get_index_name

static const char * ah_explain_get_index_name_hook(Oid indexId)
{
	const char *result;

	if (ah_original_explain_get_index_name_hook)
	{
		result = ah_original_explain_get_index_name_hook(indexId);
	}
	else {
		result = NULL;
	}
	if (result == NULL)
	{
		result = get_rel_name(indexId);
		if (result == NULL)
		{
			elog(ERROR, "cache lookup failed for index %u", indexId);
		}
	}

	elog(WARNING, "explain_get_index_name_hook called: %s",result);
	return result;

}

#if PG_VERSION_NUM >= 180000
// explain_validate_options_hook

static void ah_explain_validate_options_hook (struct ExplainState *es,
										   	List *options,
											ParseState *pstate)
{
	elog(WARNING, "explain_validate_options_hook called");

}
#endif


// --------------------------------------
// --------------------------------------
// --------------------------------------

#ifdef PG_MODULE_MAGIC_EXT
PG_MODULE_MAGIC_EXT(.name = "all_hooks", .version = "0.1.0");
#else
PG_MODULE_MAGIC;
#endif


void _PG_init(void);
void _PG_fini(void);

/* Function definitions */


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


	// explain_hook
#if PG_VERSION_NUM >= 180000
	// per_node_hook
	elog(WARNING,"hooking: explain_per_node_hook");
	ah_original_explain_per_node_hook = explain_per_node_hook;
	explain_per_node_hook = ah_explain_per_node_hook;

	// per_plan_hook
	elog(WARNING,"hooking: explain_per_plan_hook");
	ah_original_explain_per_plan_hook = explain_per_plan_hook;
	explain_per_plan_hook = ah_explain_per_plan_hook;

	// set_rel_pathlist_hook
	elog(WARNING,"hooking: set_rel_pathlist_hook");
	ah_original_set_rel_pathlist_hook = set_rel_pathlist_hook;
	set_rel_pathlist_hook = ah_set_rel_pathlist_hook;

#endif

	// object_access_hook
	elog(WARNING,"hooking: object_access_hook");
	ah_original_object_access_hook = object_access_hook;
	object_access_hook = ah_object_access_hook;

	// object_access_hook_str
	elog(WARNING,"hooking: object_access_hook_str");
	ah_original_object_access_hook_str = object_access_hook_str;
	object_access_hook_str = ah_object_access_hook_str;

	// explain_get_index_name_hook
	elog(WARNING,"hooking: explain_get_index_name_hook");
	ah_original_explain_get_index_name_hook = explain_get_index_name_hook;
	explain_get_index_name_hook = ah_explain_get_index_name_hook;

#if PG_VERSION_NUM >= 180000
	// explain_validate_options_hook
	elog(WARNING,"hooking: explain_validate_options_hook");
	ah_original_explain_validate_option_hook = explain_validate_options_hook;
	explain_validate_options_hook = ah_explain_validate_options_hook;
#endif

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
#if PG_VERSION_NUM >= 180000
	explain_per_plan_hook = ah_original_explain_per_plan_hook;
	explain_per_node_hook = ah_original_explain_per_node_hook;
	set_rel_pathlist_hook = ah_original_set_rel_pathlist_hook;
	explain_get_index_name_hook = ah_original_explain_get_index_name_hook;
	explain_validate_options_hook = ah_original_explain_validate_option_hook;
#endif
}
