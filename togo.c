/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2016 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:   me@michaellee.cn                                           |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_togo.h"
#include <zend_exceptions.h>

static int le_togo_sock;
static zend_class_entry *togo_ce;
static zend_class_entry *togo_exception_ce;
static zend_class_entry *spl_ce_RuntimeException;

const zend_function_entry togo_methods[] = {
	PHP_ME(Togo, __construct, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Togo, __destruct, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Togo, connect, NULL , ZEND_ACC_PUBLIC)
	PHP_ME(Togo, write, NULL , ZEND_ACC_PUBLIC)
	PHP_ME(Togo, version, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Togo, counter_plus, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Togo, counter_reset, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Togo, counter_get, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Togo, counter_minus, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Togo, lock_lock, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Togo, lock_unlock, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Togo, lock_status, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Togo, queue_lpush, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Togo, queue_rpush, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Togo, queue_lpop, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Togo, queue_rpop, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Togo, queue_count, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Togo, queue_status, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Togo, close, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

PHP_INI_BEGIN()
    PHP_INI_ENTRY("Togo.version", PHP_TOGO_VERSION, PHP_INI_ALL, NULL)
	PHP_INI_ENTRY("Togo.author", PHP_TOGO_AUTHOR, PHP_INI_ALL, NULL)
PHP_INI_END()

/**
 * 销毁togo_sock
 * @param zend_rsrc_list_entry *rsrc
 * @return void
 */
static void togo_destructor_togo_sock(zend_rsrc_list_entry *rsrc TSRMLS_CC)
{
	TogoSock *ts = (TogoSock *) rsrc->ptr;
	togo_sock_disconnect(ts TSRMLS_CC);
	togo_sock_free(ts);
}
/**
 * 销毁togo_sock
 * @param TogoSock *ts
 * @return void
 */
PHPAPI void togo_sock_free(TogoSock *ts)
{
	efree(ts->host);
	efree(ts);
}
/**
 * togo异常基础
 * @param int root
 * @return exception
 */
PHPAPI zend_class_entry *togo_get_exception_base(int root TSRMLS_CC)
{
#if defined(HAVE_SPL)
	if (!root) {
		if (!spl_ce_RuntimeException) {
			zend_class_entry **pce;

			if (zend_hash_find(CG(class_table), "runtimeexception", sizeof("RuntimeException"), (void **) &pce) == SUCCESS) {
				spl_ce_RuntimeException = *pce;
				return *pce;
			}
		} else {
			return spl_ce_RuntimeException;
		}
	}
#endif
#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 2)
	return zend_exception_get_default();
#else
	return zend_exception_get_default(TSRMLS_C);
#endif
}
/**
 * togo异常基础
 * @param string host
 * @param int host_len
 * @param short port
 * @param long timeout
 * @return exception
 */
PHPAPI TogoSock *togo_sock_create(char *host,int host_len ,unsigned short port,long timeout)
{
	TogoSock *ts;
	ts = emalloc(sizeof *ts);
	ts->host = emalloc(host_len + 1);
	ts->stream = NULL;
	ts->state = TOGO_SOCK_STATE_DISCONNECTED;
	memcpy(ts->host ,host ,host_len);
	ts->timeout = timeout;
	ts->host[host_len] = '\0';
	ts->port = port;
	return ts;
}
/**
 * togo异常基础
 * @param TogoSock ts
 * @return TogoSock
 */
PHPAPI int togo_sock_connect(TogoSock *ts TSRMLS_DC)
{
	struct timeval tv,*tv_ptr = NULL;
	char *host ,*hash_key ,*errstr = NULL;
	int host_len ,err = 0;

	if(ts->stream != NULL){
		togo_sock_disconnect(ts TSRMLS_CC);
	}
	tv.tv_sec = ts->timeout;
	tv.tv_usec = 0;
	host_len = spprintf(&host ,0 ,"%s:%u" ,ts->host ,ts->port);

	if(tv.tv_sec != 0){
		tv_ptr = &tv;
	}

	ts->stream = php_stream_xport_create(
		host,
		host_len,
		ENFORCE_SAFE_MODE,
		STREAM_XPORT_CLIENT | STREAM_XPORT_CONNECT,
		hash_key,
		tv_ptr,
		NULL,
		&errstr,
		&err
	);
	efree(host);
	if(!ts->stream)
	{
		efree(errstr);
		return FAILURE;
	}
	php_stream_auto_cleanup(ts->stream);

	if(tv.tv_sec != 0)
	{
		php_stream_set_option(
			ts->stream,
			PHP_STREAM_OPTION_READ_TIMEOUT,
			0,
			&tv
		);
	}
	php_stream_set_option(
		ts->stream,
		PHP_STREAM_OPTION_WRITE_BUFFER,
		PHP_STREAM_BUFFER_NONE,
		NULL
	);
	ts->state = TOGO_SOCK_STATE_CONNECTED;
	return SUCCESS;
}
/**
 * togo异常基础
 * @param TogoSock ts
 * @oaram int force_connect
 * @return int
 */
PHPAPI int togo_sock_open_server(TogoSock *ts ,int force_connect TSRMLS_DC)
{
	int res;
	res = FAILURE;
	switch(ts->state)
	{
		case TOGO_SOCK_STATE_FAILED:
			break;
		case TOGO_SOCK_STATE_DISCONNECTED:
			return togo_sock_connect(ts TSRMLS_CC);
			break;
		case TOGO_SOCK_STATE_UNKONW:
			if(!force_connect || togo_sock_connect(ts TSRMLS_CC) == SUCCESS)
			{
				res = SUCCESS;
				ts->state = TOGO_SOCK_STATE_CONNECTED;
			}
			break;
		case TOGO_SOCK_STATE_CONNECTED:
			res = SUCCESS;
			break;
		default:
			break;
	}
	return res;
}
/**
 * togo sock写入
 * @param TogoSock ts
 * @param char cmd
 * @param size_t sz
 * @return int
 */
PHPAPI int togo_sock_write(TogoSock *ts ,char *cmd ,size_t sz TSRMLS_DC)
{
	togo_sock_eof(ts TSRMLS_CC);
	return php_stream_write(ts->stream ,cmd ,sz);
}
/**
 * togo sock end of file
 * @param TogoSock ts
 * @return int
 */
PHPAPI void togo_sock_eof(TogoSock *ts TSRMLS_DC)
{
    int eof = php_stream_eof(ts->stream);
    while(eof)
    {
    	ts->stream = NULL;
        togo_sock_connect(ts TSRMLS_CC);
        eof = php_stream_eof(ts->stream);
    }
}
/**
 * togo_sock获取
 * @param TogoSock ts
 * @param zval id
 * @return int
 */
PHPAPI int togo_sock_get(TogoSock **ts ,zval *id TSRMLS_DC)
{

    zval **socket;
    int resource_type;
    if (Z_TYPE_P(id) != IS_OBJECT ||
    	zend_hash_find(
    		Z_OBJPROP_P(id),
    		"togo_sock",
			sizeof("togo_sock"),
			(void **) &socket
     ) == FAILURE)
    {
        return FAILURE;
    }

    *ts = (TogoSock *) zend_list_find(Z_LVAL_PP(socket), &resource_type);
    if (!*ts || resource_type != le_togo_sock)
    {
    	return FAILURE;
    }
    return Z_LVAL_PP(socket);
}
/**
 * togo_sock_read
 * @param TogoSock ts
 * @param int buf_len
 */
PHPAPI char *togo_sock_read(TogoSock *ts, int *buf_len TSRMLS_DC)
{
	char buf[1024];
	char *res;
    int len = 0;
    togo_sock_eof(ts TSRMLS_CC);
    php_stream_read(ts->stream, buf, 1024);
	res = togo_sock_response_parser(buf TSRMLS_CC);
	*buf_len = strlen(res);
	return res;
}
/**
 *togo_sock_response_parser
 *@param char inbuf
 */
PHPAPI char * togo_sock_response_parser(char *inbuf TSRMLS_DC)
{
	char *string, *ptr , *strbegin, *strend;

	spprintf(&strbegin, 0, "%s", TOGO_RESPONSE_MSG_HEADER);
	spprintf(&strend, 0, "%s%s", TOGO_RESPONSE_MSG_END ,RESPONSE_END_SYMBOL);

	int beginindex,endindex,beginstrlength=strlen(strbegin);

	ptr = strstr(inbuf, strbegin);
	beginindex=ptr-inbuf;
	ptr = strstr(inbuf, strend);

	endindex=ptr-inbuf;
	int n=endindex-beginindex-beginstrlength;

	if(n>0)
	{
		string=(char*)emalloc((n + 1)*sizeof(char));
		strncpy(string, inbuf+beginindex+beginstrlength, n);
		string[n]='\0';
		return string;
	}
	return NULL;
}
/**
 * togo_sock_response_state_check
 * @param char response
 */
PHPAPI int togo_sock_response_state_check(char *response TSRMLS_DC)
{
	if(response ==  NULL){
		zend_throw_exception_ex( togo_exception_ce ,TOGO_EXCEPTION_CODE_PROTOCOL_ERR TSRMLS_CC ,TOGO_EXCEPTION_MSG_PROTOCOL_ERR);
		return FAILURE;
	}
	if(strcmp(TOGO_RESPONSE_MSG_FAIL ,response) == 0)//fail
	{
		return FAILURE;
	}else if(strcmp(TOGO_RESPONSE_MSG_NULL ,response) == 0)//null
	{
		return FAILURE;
	}else if(strcmp(TOGO_RESPONSE_MSG_COMMANDBIG ,response) == 0)//command is too big
	{
		return FAILURE;
	}else if(strcmp(TOGO_RESPONSE_MSG_BIG ,response) == 0)//request data or response data is too big
	{
		return FAILURE;
	}else if(strcmp(TOGO_RESPONSE_MSG_IS_EXIST ,response) == 0)//element is exist
	{
		return FAILURE;
	}else if(strcmp(TOGO_RESPONSE_MSG_NOT_EXIST ,response) == 0)//element is not exist
	{
		return FAILURE;
	}else if(strcmp(TOGO_RESPONSE_MSG_OK ,response) == 0)//response success
	{
		return SUCCESS;
	}else
	{
		return SUCCESS;
	}
}
/**
 * togo关闭链接
 * @param TogoSock ts
 * @return int
 */
PHPAPI int togo_sock_disconnect(TogoSock *ts TSRMLS_DC)
{
	int res = FAILURE;
	if(ts->stream != NULL)
	{
		ts->state = TOGO_SOCK_STATE_DISCONNECTED;
		php_stream_close(ts->stream);
		ts->stream = NULL;
		res = SUCCESS;
	}
	return res;
}
/**
 * togo关闭
 * @param TogoSock ts
 * @return int
 */
PHPAPI int togo_sock_close(zval *object)
{
	TogoSock *ts;
	zval **socket;
	if (togo_sock_get(&ts ,object TSRMLS_CC) == FAILURE)
	{
		return FAILURE;
	}
	char *cmd = "quit";
	size_t cmd_len;
	cmd_len = spprintf(&cmd ,0 ,"%s%s" ,cmd ,CMD_END_SYMBOL);
	togo_sock_write(ts ,cmd ,cmd_len);
	if (togo_sock_disconnect(ts TSRMLS_CC) == FAILURE)
	{
		return FAILURE;
	}
    if (Z_TYPE_P(object) != IS_OBJECT ||
    	zend_hash_find(
    		Z_OBJPROP_P(object),
    		"togo_sock",
			sizeof("togo_sock"),
			(void **) &socket
     ) == FAILURE)
    {
        return FAILURE;
    }
	zend_list_delete(Z_LVAL_PP(socket));
	return SUCCESS;
}
//module init
PHP_MINIT_FUNCTION(togo)
{
	//注册全局配置
	REGISTER_INI_ENTRIES();
	spl_ce_RuntimeException = NULL;
	//Togo class
	zend_class_entry togo_class_entry;
	INIT_CLASS_ENTRY(togo_class_entry ,"Togo" ,togo_methods);
	togo_ce = zend_register_internal_class(&togo_class_entry TSRMLS_CC);
	//Togo exception class
	zend_class_entry togo_exception_class_entry;
	INIT_CLASS_ENTRY(togo_exception_class_entry ,"TogoException" ,NULL);
	togo_exception_ce = zend_register_internal_class_ex(
		&togo_exception_class_entry,
		togo_get_exception_base(0 TSRMLS_CC),
		NULL
		TSRMLS_CC
	);
	le_togo_sock = zend_register_list_destructors_ex(
		togo_destructor_togo_sock,
		NULL,
		TOGO_SOCK_NAME,
		module_number
	);
	return SUCCESS;
}
PHP_MSHUTDOWN_FUNCTION(togo)
{
	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}
PHP_RINIT_FUNCTION(togo)
{
	return SUCCESS;
}
PHP_RSHUTDOWN_FUNCTION(togo)
{
	return SUCCESS;
}
PHP_MINFO_FUNCTION(togo)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "Version", PHP_TOGO_VERSION);
	php_info_print_table_header(2, "Author", PHP_TOGO_AUTHOR);
	php_info_print_table_end();
}

zend_module_entry togo_module_entry = {
	STANDARD_MODULE_HEADER,
	"togo",
	togo_methods,
	PHP_MINIT(togo),
	PHP_MSHUTDOWN(togo),
	PHP_RINIT(togo),
	PHP_RSHUTDOWN(togo),
	PHP_MINFO(togo),
	PHP_TOGO_VERSION,
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_TOGO
ZEND_GET_MODULE(togo)
#endif
PHP_METHOD(Togo ,__construct){
	RETURN_TRUE;
}
/**
 * togo destruct
 */
PHP_METHOD(Togo ,__destruct){
	zval *object = getThis();
	togo_sock_close(object);
}
/**
 * togo connect method
 */
PHP_METHOD(Togo ,connect){
	zval *object;
	char *host = NULL;
	int host_len, id;
	long port;

	struct timeval tv = {0L ,0L};
	TogoSock *ts = NULL;
	//parameter
	if(zend_parse_method_parameters(
		ZEND_NUM_ARGS()
		TSRMLS_CC,
		getThis(),
		"Osl|l",
		&object,
		togo_ce,
		&host,
		&host_len,
		&port,
		&tv.tv_sec
	 ) == FAILURE){
		zend_throw_exception_ex(
			togo_exception_ce,
			TOGO_EXCEPTION_CODE_PARAM
			TSRMLS_CC,
			TOGO_EXCEPTION_MSG_PARAM,
			"connect"
		);
		RETURN_FALSE;
	}

	if(host_len == 0)
	{
		char *p;
		*p = TOGO_SOCK_DEFAULT_HOST;
		host = &p;
		host_len = strlen(p);
		efree(p);
	}

	port = port ? port : TOGO_SOCK_DEFAULT_PORT;

	tv.tv_sec = tv.tv_sec ? tv.tv_sec : TOGO_SOCK_DEFAULT_TIMEOUT;

	//Timeout error
	if(tv.tv_sec <= 0 || tv.tv_sec > INT_MAX)
	{

		zend_throw_exception_ex(
			togo_exception_ce,
			TOGO_EXCEPTION_CODE_TIMEOUT
			TSRMLS_CC,
			TOGO_EXCEPTION_MSG_TIMEOUT,
			"connect",
			tv.tv_sec
		);
		RETURN_FALSE;
	}
	ts = togo_sock_create(host ,host_len ,port ,tv.tv_sec);
	if(togo_sock_open_server(ts ,1 TSRMLS_CC) == FAILURE)
	{
		zend_throw_exception_ex(
			togo_exception_ce,
			TOGO_EXCEPTION_CODE_CONNECT_FAIL
			TSRMLS_CC,
			TOGO_EXCEPTION_MSG_CONNECT_FAIL,
			"connect",
			host,
			port,
			tv.tv_sec
		);
		RETURN_FALSE;
	}
	id = zend_list_insert(ts ,le_togo_sock TSRMLS_CC);
	add_property_resource(object ,"togo_sock" ,id);
	php_stream_to_zval(ts->stream ,return_value);
}
/**
 * togo write method
 */
PHP_METHOD(Togo, write)
{
	zval *object = getThis();
    TogoSock *ts;
    char *cmd,*t_cmd, *response;
    int cmd_len,t_cmd_len, response_len ,w_res;
    long mode=0;
    //togo sock get
    if (togo_sock_get(&ts ,object TSRMLS_CC) == FAILURE)
    {
    	zend_throw_exception_ex(
			togo_exception_ce,
			TOGO_EXCEPTION_CODE_CONN_ERR
			TSRMLS_CC,
			TOGO_EXCEPTION_MSG_CONN_ERR,
			"write"
    	);
        RETURN_FALSE;
    }
	//parameter
	if(zend_parse_parameters(
		ZEND_NUM_ARGS()
		TSRMLS_CC,
		"s|l",
		&cmd,
		&cmd_len,
		&mode
	 ) == FAILURE){
		zend_throw_exception_ex(
			togo_exception_ce,
			TOGO_EXCEPTION_CODE_PARAM
			TSRMLS_CC,
			TOGO_EXCEPTION_MSG_PARAM,
			"write"
		);
		RETURN_FALSE;
	}
	if(mode == 0)
	{
		t_cmd_len = spprintf(&t_cmd ,0 ,"%s%s" ,cmd ,CMD_END_SYMBOL);
		w_res = togo_sock_write(ts, t_cmd, t_cmd_len TSRMLS_CC);
	}else{
		w_res = togo_sock_write(ts, cmd, cmd_len TSRMLS_CC);
	}
    //sock write
    if (w_res < 0)
    {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    response = togo_sock_read(ts, &response_len TSRMLS_CC);
    //sock read
    if (response == NULL)
    {
        RETURN_FALSE;
    }
    RETURN_STRINGL(response, response_len, 0);
}
/**
 * version
 */
PHP_METHOD(Togo, version)
{
	zval *object = getThis();
    TogoSock *ts;
    char *cmd, *response;
    int cmd_len, response_len ,w_res;
    long mode=0;
    //togo sock get
    if (togo_sock_get(&ts ,object TSRMLS_CC) == FAILURE)
    {
    	zend_throw_exception_ex(
			togo_exception_ce,
			TOGO_EXCEPTION_CODE_CONN_ERR
			TSRMLS_CC,
			TOGO_EXCEPTION_MSG_CONN_ERR,
			"version"
    	);
        RETURN_FALSE;
    }

	cmd_len = spprintf(&cmd ,0 ,"VERSION%s" ,CMD_END_SYMBOL);
	w_res = togo_sock_write(ts, cmd, cmd_len TSRMLS_CC);
    //sock write
    if (w_res < 0)
    {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    response = togo_sock_read(ts, &response_len TSRMLS_CC);
    if(togo_sock_response_state_check(response) == FAILURE)
    {
    	RETURN_FALSE;
    }
    RETURN_STRINGL(response, response_len, 0);
}
/**
 * counter_get
 */
PHP_METHOD(Togo, counter_get)
{
	zval *object = getThis();
    TogoSock *ts;
    char *cmd,*key, *response;
    int cmd_len,response_len ,w_res,key_len;
    long mode=0;
    //togo sock get
    if (togo_sock_get(&ts ,object TSRMLS_CC) == FAILURE)
    {
    	zend_throw_exception_ex(
			togo_exception_ce,
			TOGO_EXCEPTION_CODE_CONN_ERR
			TSRMLS_CC,
			TOGO_EXCEPTION_MSG_CONN_ERR,
			"counter_get"
    	);
        RETURN_FALSE;
    }
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",&key, &key_len) == FAILURE)
    {
        RETURN_FALSE;
    }
	cmd_len = spprintf(&cmd ,0 ,"counter_get %s%s" ,CMD_END_SYMBOL);
	w_res = togo_sock_write(ts, cmd, cmd_len TSRMLS_CC);
    //sock write
    if (w_res < 0)
    {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    response = togo_sock_read(ts, &response_len TSRMLS_CC);
    //sock read
    if(togo_sock_response_state_check(response) == FAILURE)
    {
    	RETURN_FALSE;
    }
    RETURN_LONG(atol(response));
}
/**
 * counter_plus
 */
PHP_METHOD(Togo, counter_plus)
{
	zval *object = getThis();
    TogoSock *ts;
    char *cmd , *response ,*key = NULL;
    int cmd_len , response_len ,key_len;
    long value = 1 ;
    //togo sock get
    if (togo_sock_get(&ts ,object TSRMLS_CC) < 0)
    {
        RETURN_FALSE;
    }
    //param
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l",&key, &key_len ,&value) == FAILURE)
    {
        RETURN_FALSE;
    }
    //cmd
    if(value <= 0)
    {
    	cmd_len = spprintf(&cmd, 0, "COUNTER PLUS %s %d%s", key ,1 ,CMD_END_SYMBOL);
    }else{
    	cmd_len = spprintf(&cmd, 0, "COUNTER PLUS %s %ld%s", key ,value ,CMD_END_SYMBOL);
    }
    //sock write
    if (togo_sock_write(ts, cmd, cmd_len TSRMLS_CC) < 0)
    {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    response = togo_sock_read(ts, &response_len TSRMLS_CC);
    //sock read
    if(togo_sock_response_state_check(response) == FAILURE)
    {
    	RETURN_FALSE;
    }
    RETURN_LONG(atol(response));
}
/**
 * counter_reset
 */
PHP_METHOD(Togo, counter_reset)
{
    zval *object = getThis();
    TogoSock *ts;
    char *cmd , *response ,*key = NULL;
    int cmd_len , response_len ,key_len;
    long value = 1 ;
    //togo sock get
    if (togo_sock_get(&ts ,object TSRMLS_CC) < 0)
    {
        RETURN_FALSE;
    }
    //param
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l",&key, &key_len ,&value) == FAILURE)
    {
        RETURN_FALSE;
    }
    //cmd
    if(value <= 0)
    {
        cmd_len = spprintf(&cmd, 0, "COUNTER RESET %s %d%s", key ,1 ,CMD_END_SYMBOL);
    }else{
        cmd_len = spprintf(&cmd, 0, "COUNTER RESET %s %ld%s", key ,value ,CMD_END_SYMBOL);
    }
    //sock write
    if (togo_sock_write(ts, cmd, cmd_len TSRMLS_CC) < 0)
    {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    response = togo_sock_read(ts, &response_len TSRMLS_CC);
    //check
    if(togo_sock_response_state_check(response) == FAILURE)
    {
    	RETURN_FALSE;
    }
    //sock read
    if (atol(response) == 0)
    {
        RETURN_TRUE;
    }
    RETURN_FALSE;
}
/**
 * counter_minus
 */
PHP_METHOD(Togo, counter_minus)
{
	zval *object = getThis();
    TogoSock *ts;
    char *cmd , *response ,*key = NULL;
    int cmd_len , response_len ,key_len;
    long value = 1 ;
    //togo sock get
    if (togo_sock_get(&ts ,object TSRMLS_CC) < 0)
    {
        RETURN_FALSE;
    }
    //param
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l",&key, &key_len ,&value) == FAILURE)
    {
        RETURN_FALSE;
    }
    //cmd
    if(value <= 0)
    {
    	cmd_len = spprintf(&cmd, 0, "COUNTER MINUS %s %d%s", key ,1 ,CMD_END_SYMBOL);
    }else{
    	cmd_len = spprintf(&cmd, 0, "COUNTER MINUS %s %ld%s", key ,value ,CMD_END_SYMBOL);
    }
    //sock write
    if (togo_sock_write(ts, cmd, cmd_len TSRMLS_CC) < 0)
    {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    response = togo_sock_read(ts, &response_len TSRMLS_CC);
    //sock read
    if(togo_sock_response_state_check(response) == FAILURE)
    {
    	RETURN_FALSE;
    }
    RETURN_LONG(atol(response));
}
/**
 * lock_lock
 */
PHP_METHOD(Togo, lock_lock)
{
    zval *object = getThis();
    TogoSock *ts;
    char *cmd , *response ,*key = NULL;
    int cmd_len , response_len ,key_len;
    //togo sock get
    if (togo_sock_get(&ts ,object TSRMLS_CC) < 0)
    {
        RETURN_FALSE;
    }
    //param
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",&key, &key_len) == FAILURE)
    {
        RETURN_FALSE;
    }
    //cmd
    cmd_len = spprintf(&cmd, 0, "LOCK LOCK %s%s", key ,CMD_END_SYMBOL);
    //sock write
    if (togo_sock_write(ts, cmd, cmd_len TSRMLS_CC) < 0)
    {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    response = togo_sock_read(ts, &response_len TSRMLS_CC);
    //check
    if(togo_sock_response_state_check(response) == FAILURE)
    {
    	RETURN_FALSE;
    }
    //sock read
    if (atol(response) == 1)
    {
        RETURN_TRUE;
    }else{
        RETURN_FALSE;
    }

}
/**
 * lock_unlock
 */
PHP_METHOD(Togo, lock_unlock)
{
    zval *object = getThis();
    TogoSock *ts;
    char *cmd , *response ,*key = NULL;
    int cmd_len , response_len ,key_len;
    //togo sock get
    if (togo_sock_get(&ts ,object TSRMLS_CC) < 0)
    {
        RETURN_FALSE;
    }
    //param
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",&key, &key_len) == FAILURE)
    {
        RETURN_FALSE;
    }
    //cmd
    cmd_len = spprintf(&cmd, 0, "LOCK UNLOCK %s%s", key, CMD_END_SYMBOL);
    //sock write
    if (togo_sock_write(ts, cmd, cmd_len TSRMLS_CC) < 0)
    {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    response = togo_sock_read(ts, &response_len TSRMLS_CC);
    if(togo_sock_response_state_check(response) == FAILURE)
    {
    	RETURN_FALSE;
    }
    //sock read
    if (atol(response) == 0)
    {
        RETURN_TRUE;
    }else{
        RETURN_FALSE;
    }
}
/**
 * lock_status
 */
PHP_METHOD(Togo, lock_status)
{
    zval *object = getThis();
    TogoSock *ts;
    char *cmd , *response ,*key = NULL;
    int cmd_len , response_len ,key_len;
    //togo sock get
    if (togo_sock_get(&ts ,object TSRMLS_CC) < 0)
    {
        RETURN_FALSE;
    }
    //param
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",&key, &key_len) == FAILURE)
    {
        RETURN_FALSE;
    }
    //cmd
    cmd_len = spprintf(&cmd, 0, "LOCK STATUS %s%s", key, CMD_END_SYMBOL);
    //sock write
    if (togo_sock_write(ts, cmd, cmd_len TSRMLS_CC) < 0)
    {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    response = togo_sock_read(ts, &response_len TSRMLS_CC);
    if(togo_sock_response_state_check(response) == FAILURE)
    {
    	RETURN_FALSE;
    }
    //sock read
    RETURN_STRINGL(response, response_len, 0);
}
/**
 * queue_lpush
 */
PHP_METHOD(Togo, queue_lpush)
{
    zval *object = getThis();
    TogoSock *ts;
    char *cmd , *response ,*key = NULL;
    int cmd_len , response_len ,key_len;
    long value ,priority = 0;
    //togo sock get
    if (togo_sock_get(&ts ,object TSRMLS_CC) < 0)
    {
        RETURN_FALSE;
    }
    //param
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl|l",&key, &key_len ,&value ,&priority) == FAILURE)
    {
        RETURN_FALSE;
    }
    //cmd
    if( priority != 0)
    {
        cmd_len = spprintf(&cmd, 0, "QUEUE LPUSH %s %ld %ld%s", key ,value ,priority ,CMD_END_SYMBOL);
    } else {
        cmd_len = spprintf(&cmd, 0, "QUEUE LPUSH %s %ld%s", key ,value ,CMD_END_SYMBOL);
    }
    //sock write
    if (togo_sock_write(ts, cmd, cmd_len TSRMLS_CC) < 0)
    {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    response = togo_sock_read(ts, &response_len TSRMLS_CC);
    if(togo_sock_response_state_check(response) == FAILURE)
    {
    	RETURN_FALSE;
    }
    //sock read
    RETURN_TRUE;
}
/**
 * queue_rpush
 */
PHP_METHOD(Togo, queue_rpush)
{
    zval *object = getThis();
    TogoSock *ts;
    char *cmd , *response ,*key = NULL;
    int cmd_len , response_len ,key_len;
    long value ,priority = 0;
    //togo sock get
    if (togo_sock_get(&ts ,object TSRMLS_CC) < 0)
    {
        RETURN_FALSE;
    }
    //param
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl|l",&key, &key_len ,&value ,&priority) == FAILURE)
    {
        RETURN_FALSE;
    }
    //cmd
    if( priority != 0)
    {
        cmd_len = spprintf(&cmd, 0, "QUEUE RPUSH %s %ld %ld%s", key ,value ,priority ,CMD_END_SYMBOL);
    } else {
        cmd_len = spprintf(&cmd, 0, "QUEUE RPUSH %s %ld%s", key ,value ,CMD_END_SYMBOL);
    }
    //sock write
    if (togo_sock_write(ts, cmd, cmd_len TSRMLS_CC) < 0)
    {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    response = togo_sock_read(ts, &response_len TSRMLS_CC);
    if(togo_sock_response_state_check(response) == FAILURE)
    {
    	RETURN_FALSE;
    }
    //sock read
    RETURN_TRUE;
}
/**
 * queue_lpop
 */
PHP_METHOD(Togo, queue_lpop)
{
    zval *object = getThis();
    TogoSock *ts;
    char *cmd , *response ,*key = NULL;
    int cmd_len , response_len ,key_len;
    //togo sock get
    if (togo_sock_get(&ts ,object TSRMLS_CC) < 0)
    {
        RETURN_FALSE;
    }
    //param
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",&key, &key_len) == FAILURE)
    {
        RETURN_FALSE;
    }
    //cmd
    cmd_len = spprintf(&cmd, 0, "QUEUE LPOP %s%s", key, CMD_END_SYMBOL);
    //sock write
    if (togo_sock_write(ts, cmd, cmd_len TSRMLS_CC) < 0)
    {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    response = togo_sock_read(ts, &response_len TSRMLS_CC);
    if(togo_sock_response_state_check(response) == FAILURE)
    {
    	RETURN_FALSE;
    }
    RETURN_LONG(atol(response));
}
/**
 * queue_rpop
 */
PHP_METHOD(Togo, queue_rpop)
{
    zval *object = getThis();
    TogoSock *ts;
    char *cmd , *response ,*key = NULL;
    int cmd_len , response_len ,key_len;
    //togo sock get
    if (togo_sock_get(&ts ,object TSRMLS_CC) < 0)
    {
        RETURN_FALSE;
    }
    //param
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",&key, &key_len) == FAILURE)
    {
        RETURN_FALSE;
    }
    //cmd
    cmd_len = spprintf(&cmd, 0, "QUEUE RPOP %s%s", key, CMD_END_SYMBOL);
    //sock write
    if (togo_sock_write(ts, cmd, cmd_len TSRMLS_CC) < 0)
    {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    response = togo_sock_read(ts, &response_len TSRMLS_CC);
    //sock read
    if(togo_sock_response_state_check(response) == FAILURE)
    {
    	RETURN_FALSE;
    }
    RETURN_LONG(atol(response));
}
/**
 * queue_count
 */
PHP_METHOD(Togo, queue_count)
{
    zval *object = getThis();
    TogoSock *ts;
    char *cmd , *response ,*key = NULL;
    int cmd_len , response_len ,key_len;
    //togo sock get
    if (togo_sock_get(&ts ,object TSRMLS_CC) < 0)
    {
        RETURN_FALSE;
    }
    //param
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",&key, &key_len) == FAILURE)
    {
        RETURN_FALSE;
    }
    //cmd
    cmd_len = spprintf(&cmd, 0, "QUEUE COUNT %s%s", key, CMD_END_SYMBOL);
    //sock write
    if (togo_sock_write(ts, cmd, cmd_len TSRMLS_CC) < 0)
    {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    response = togo_sock_read(ts, &response_len TSRMLS_CC);
    //sock read
    if(togo_sock_response_state_check(response) == FAILURE)
    {
    	RETURN_FALSE;
    }
    RETURN_LONG(atol(response));
}
/**
 * queue_status
 */
PHP_METHOD(Togo, queue_status)
{
    zval *object = getThis();
    TogoSock *ts;
    char *cmd , *response ,*key = NULL;
    int cmd_len , response_len ,key_len;
    //togo sock get
    if (togo_sock_get(&ts ,object TSRMLS_CC) < 0)
    {
        RETURN_FALSE;
    }
    //param
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",&key, &key_len) == FAILURE)
    {
        RETURN_FALSE;
    }
    //cmd
    cmd_len = spprintf(&cmd, 0, "QUEUE STATUS %s%s", key);
    //sock write
    if (togo_sock_write(ts, cmd, cmd_len TSRMLS_CC) < 0)
    {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    response = togo_sock_read(ts, &response_len TSRMLS_CC);
    //sock read
    if(togo_sock_response_state_check(response) == FAILURE)
    {
    	RETURN_FALSE;
    }
    RETURN_STRINGL(response ,response_len ,0);
}
/**
 * togo close
 */
PHP_METHOD(Togo ,close){
	zval *object = getThis();
	if(togo_sock_close(object) == FAILURE)
	{
		RETURN_FALSE;
	}
	RETURN_TRUE;
}
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
