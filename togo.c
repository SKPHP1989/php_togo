/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2014 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Michael Lee <me@michaellee.cn>                               |
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
static zend_class_entry *spl_ce_RuntimeException = NULL;

/* If you declare any globals in php_togo.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(togo)
*/

/* True global resources - no need for thread safety here */
static int le_togo;

/* {{{ togo_functions[]
 *
 * Every user visible function must have an entry in togo_functions[].
 */
const zend_function_entry togo_functions[] = {
	PHP_ME(Togo, __construct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Togo, __destruct, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Togo, connect, NULL, ZEND_ACC_PUBLIC)
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
	PHP_FE_END	/* Must be the last line in togo_functions[] */
};
/* }}} */

/* {{{ togo_module_entry
 */
zend_module_entry togo_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"togo",
	togo_functions,
	PHP_MINIT(togo),
	PHP_MSHUTDOWN(togo),
	PHP_RINIT(togo),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(togo),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(togo),
#if ZEND_MODULE_API_NO >= 20010901
	PHP_TOGO_VERSION,
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_TOGO
ZEND_GET_MODULE(togo)
#endif
/**
 * register class constact
 */
void add_constant_long(zend_class_entry *ce, char *name, int value)
{
	zval *constval;
	constval = pemalloc(sizeof(zval), 1);
	INIT_PZVAL(constval);
	ZVAL_LONG(constval, value);
	zend_hash_add(&ce->constants_table, name, 1 + strlen(name),
		(void*)&constval, sizeof(zval*), NULL);
}

/**
 * togo_sock_create
 */
PHPAPI TogoSock* togo_sock_create(char *host, int host_len, unsigned short port, long timeout)
{
    TogoSock *togo_sock;

    togo_sock         = emalloc(sizeof *togo_sock);
    togo_sock->host   = emalloc(host_len + 1);
    togo_sock->stream = NULL;
    togo_sock->status =TOGO_SOCK_STATUS_DISCONNECTED;

    memcpy(togo_sock->host, host, host_len);
    togo_sock->host[host_len] = '\0';

    togo_sock->port    = port;
    togo_sock->timeout = timeout;

    return togo_sock;
}
/**
 * togo_sock_connect
 */
PHPAPI int togo_sock_connect(TogoSock *togo_sock TSRMLS_DC)
{
    struct timeval tv, *tv_ptr = NULL;
    char *host = NULL, *hash_key = NULL, *errstr = NULL;
    int host_len, err = 0;

    if (togo_sock->stream != NULL) {
        togo_sock_disconnect(togo_sock TSRMLS_CC);
    }

    tv.tv_sec  = togo_sock->timeout;
    tv.tv_usec = 0;

    host_len = spprintf(&host, 0, "%s:%d", togo_sock->host, togo_sock->port);

    if(tv.tv_sec != 0) {
        tv_ptr = &tv;
    }
    togo_sock->stream = php_stream_xport_create(host, host_len, ENFORCE_SAFE_MODE,
                                                 STREAM_XPORT_CLIENT | STREAM_XPORT_CONNECT,
                                                 hash_key, tv_ptr, NULL, &errstr, &err);

    efree(host);

    if (!togo_sock->stream)
    {
        efree(errstr);
        return -1;
    }

    php_stream_auto_cleanup(togo_sock->stream);

    if(tv.tv_sec != 0)
    {
        php_stream_set_option(togo_sock->stream, PHP_STREAM_OPTION_READ_TIMEOUT,
                              0, &tv);
    }
    php_stream_set_option(togo_sock->stream,
                          PHP_STREAM_OPTION_WRITE_BUFFER,
                          PHP_STREAM_BUFFER_NONE, NULL);

    togo_sock->status = TOGO_SOCK_STATUS_CONNECTED;

    return 0;
}

/**
 * togo_sock_server_open
 */
PHPAPI int togo_sock_server_open(TogoSock *togo_sock, int force_connect TSRMLS_DC)
{
    int res = -1;

    switch (togo_sock->status)
    {
        case TOGO_SOCK_STATUS_DISCONNECTED:
            return togo_sock_connect(togo_sock TSRMLS_CC);
        case TOGO_SOCK_STATUS_CONNECTED:
            res = 0;
        break;
        case TOGO_SOCK_STATUS_UNKNOWN:
            if (force_connect > 0 && togo_sock_connect(togo_sock TSRMLS_CC) < 0)
            {
                res = -1;
            } else {
                res = 0;
                togo_sock->status = TOGO_SOCK_STATUS_CONNECTED;
            }
        break;
    }
    return res;
}

/**
 * togo_free_socket
 */
PHPAPI void togo_free_socket(TogoSock *togo_sock)
{
    efree(togo_sock->host);
    efree(togo_sock);
}

/**
 * togo_sock_disconnect
 */
PHPAPI int togo_sock_disconnect(TogoSock *togo_sock TSRMLS_DC)
{
    int res = 0;
    if (togo_sock->stream != NULL)
    {
    	togo_sock_write(togo_sock, "QUIT\r\n", strlen("QUIT\r\n") TSRMLS_CC);
        togo_sock->status = TOGO_SOCK_STATUS_DISCONNECTED;
        php_stream_close(togo_sock->stream);
        togo_sock->stream = NULL;
        res = 1;
    }
    return res;
}

/**
 * togo_sock_get
 */
PHPAPI int togo_sock_get(zval *id, TogoSock **togo_sock TSRMLS_DC)
{

    zval **socket;
    int resource_type;
    if (Z_TYPE_P(id) != IS_OBJECT || zend_hash_find(Z_OBJPROP_P(id), "socket",
                                  sizeof("socket"), (void **) &socket) == FAILURE)
    {
        return -1;
    }

    *togo_sock = (TogoSock *) zend_list_find(Z_LVAL_PP(socket), &resource_type);

    if (!*togo_sock || resource_type != le_togo_sock)
    {
            return -1;
    }

    return Z_LVAL_PP(socket);
}

/**
 * togo_sock_write
 */
PHPAPI int togo_sock_write(TogoSock *togo_sock, char *cmd, size_t sz TSRMLS_DC)
{
    togo_check_eof(togo_sock TSRMLS_CC);
    return php_stream_write(togo_sock->stream, cmd, sz);
    return 0;
}

/**
 * togo_sock_read
 */
PHPAPI char *togo_sock_read(TogoSock *togo_sock, int *buf_len TSRMLS_DC)
{
	
    char inbuf[1024];
    char *resp = NULL;
    int len = 0;
    php_stream *test;
    togo_check_eof(togo_sock TSRMLS_CC);
	php_stream_gets(togo_sock->stream, inbuf, 1024);

	if(togo_sock_response_status_check(inbuf TSRMLS_CC) <0)
	{
		return NULL;	
	}
	*buf_len = strlen(inbuf) - 2;
	if(*buf_len >= 0)
	{
		resp = togo_sock_response_parser(inbuf TSRMLS_CC);
		*buf_len = strlen(resp);
		return resp;
    } else {
        zend_throw_exception_ex( togo_exception_ce ,0 TSRMLS_CC,"protocol error \n");
        return NULL;
    }
}

/**
 * togo_check_eof
 */
PHPAPI void togo_check_eof(TogoSock *togo_sock TSRMLS_DC)
{
    int eof = php_stream_eof(togo_sock->stream);
    while(eof)
    {
        togo_sock->stream = NULL;
        togo_sock_connect(togo_sock TSRMLS_CC);
        eof = php_stream_eof(togo_sock->stream);
    }
}

/**
 * togo_sock_reponse_status_check
 */
PHPAPI int togo_sock_response_status_check(char *inbuf TSRMLS_DC)
{

    if(strcmp("TOGO_STOGO_FAILTOGO_E\r\n" ,inbuf) == 0)//操作失败
    {

	    zend_throw_exception_ex( togo_exception_ce ,0 TSRMLS_CC,"TOGO_STOGO_FAILTOGO_E");
		return -1;
	}else if(strcmp("TOGO_STOGO_COMMAND_TOO_BIGTOGO_E\r\n" ,inbuf) == 0){//命令行太长

	   	zend_throw_exception_ex( togo_exception_ce ,0 TSRMLS_CC,"TOGO_STOGO_COMMAND_TOO_BIGTOGO_E");
		return -1;
	}else if(strcmp("TOGO_STOO_BIGTOGO_E\r\n" ,inbuf) == 0){//需要发送/接收的内容太大

		zend_throw_exception_ex( togo_exception_ce ,0 TSRMLS_CC,"TOGO_STOO_BIGTOGO_E");
		return -1;
	}else{

		return 0;
	}

}

/**
 * togo_sock_response_parser
 */
PHPAPI char * togo_sock_response_parser(char *inbuf TSRMLS_DC)
{
	char *string, *ptr , *strbegin, *strend;

	spprintf(&strbegin, 0, "%s", TOGO_RESPONE_HEADER);
	spprintf(&strend, 0, "%s\r\n", TOGO_RESPONE_TAIL);

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


PHPAPI zend_class_entry *togo_get_exception_base(int root TSRMLS_DC)
{
#if HAVE_SPL
        if (!root)
        {
                if (!spl_ce_RuntimeException)
                {
                        zend_class_entry **pce;
                        if (zend_hash_find(CG(class_table), "runtimeexception", sizeof("RuntimeException"), (void **) &pce) == SUCCESS)
                        {
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
 * togo_destructor_togo_sock
 */
static void togo_destructor_togo_sock(zend_rsrc_list_entry * rsrc TSRMLS_DC)
{
    TogoSock *togo_sock = (TogoSock *) rsrc->ptr;
    togo_sock_disconnect(togo_sock TSRMLS_CC);
    togo_free_socket(togo_sock);
}

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(togo)
{
	//register Togo class
	zend_class_entry togo_class_entry;
    INIT_CLASS_ENTRY(togo_class_entry, "Togo", togo_functions);
    togo_ce = zend_register_internal_class(&togo_class_entry TSRMLS_CC);
    //register TogoException class
    zend_class_entry togo_exception_class_entry;
    INIT_CLASS_ENTRY(togo_exception_class_entry, "TogoException", NULL);
    togo_exception_ce = zend_register_internal_class_ex(
        &togo_exception_class_entry,
        togo_get_exception_base(0 TSRMLS_CC),
        NULL TSRMLS_CC
    );
    //register class sock
    le_togo_sock = zend_register_list_destructors_ex(
        togo_destructor_togo_sock,
        NULL,
        togo_sock_name, module_number
    );
    //register constant
	add_constant_long(togo_ce, "TOGO_NOT_FOUND", TOGO_NOT_FOUND);
	add_constant_long(togo_ce, "TOGO_STRING", TOGO_STRING);
	add_constant_long(togo_ce, "TOGO_SET", TOGO_SET);
	add_constant_long(togo_ce, "TOGO_LIST", TOGO_LIST);
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(togo)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(togo)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(togo)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(togo)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "togo support", "enabled");
	php_info_print_table_header(2, "Author", "Michaellee");
	php_info_print_table_header(2, "version", PHP_TOGO_VERSION);
	php_info_print_table_end();
}
/* }}} */

/* {{{ proto Togo Togo::__construct()
    Public constructor */
PHP_METHOD(Togo, __construct)
{
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "") == FAILURE)
    {
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ proto Togo Togo::__destruct() Public destructer */
PHP_METHOD(Togo, __destruct)
{
    zval *object = getThis();
    TogoSock *togo_sock;
    if (togo_sock_get(object, &togo_sock TSRMLS_CC) < 0)
    {
        RETURN_FALSE;
    }
    if (togo_sock_disconnect(togo_sock TSRMLS_CC))
    {
        RETURN_TRUE;
    }
    RETURN_FALSE;
}
/* }}} */

/* {{{ proto boolean Togo::connect(string host, int port [, int timeout])
 */
PHP_METHOD(Togo, connect)
{
	zval *object;
    int host_len, id;
    char *host = NULL;
    long port;

    struct timeval timeout = {0L, 0L};
    TogoSock *togo_sock  = NULL;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osl|l",
                                     &object, togo_ce, &host, &host_len, &port,
                                     &timeout.tv_sec) == FAILURE)
    {
       RETURN_FALSE;
    }
    //time out set
    if (timeout.tv_sec < 0L || timeout.tv_sec > INT_MAX)
    {
        zend_throw_exception(togo_exception_ce, "Invalid timeout", 0 TSRMLS_CC);
        RETURN_FALSE;
    }
    //connect to server
    togo_sock = togo_sock_create(host, host_len, port, timeout.tv_sec);
    if (togo_sock_server_open(togo_sock, 1 TSRMLS_CC) < 0)
    {
        togo_free_socket(togo_sock);
        zend_throw_exception_ex(
            togo_exception_ce,
            0 TSRMLS_CC,
            "Can't connect to %s:%d",
            host,
            port
        );
        RETURN_FALSE;
    }
    id = zend_list_insert(togo_sock, le_togo_sock TSRMLS_CC);
    add_property_resource(object, "socket", id);
    php_stream_to_zval(togo_sock->stream, return_value);  
}
/* }}} */

/* {{{ proto string Togo::version()
 */
PHP_METHOD(Togo, version)
{
	zval *object = getThis();
    TogoSock *togo_sock;
    char *cmd , *response;
    int cmd_len , response_len;
    //togo sock get
    if (togo_sock_get(object, &togo_sock TSRMLS_CC) < 0)
    {	
        RETURN_FALSE;
    }
    cmd_len = spprintf(&cmd, 0, "%s\r\n", "VERSION");
    //sock write
    if (togo_sock_write(togo_sock, cmd, cmd_len TSRMLS_CC) < 0)
    {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    response = togo_sock_read(togo_sock, &response_len TSRMLS_CC);
    //sock read
    if (response == NULL)
    {
        RETURN_FALSE;
    }
    RETURN_STRINGL(response, response_len, 0);
}
/* }}} */

/* {{{ proto long Togo::counter_get()
 */
PHP_METHOD(Togo, counter_get)
{
	zval *object = getThis();
    TogoSock *togo_sock;
    char *cmd , *response ,*key = NULL;
    int cmd_len , response_len ,key_len;
    //togo sock get
    if (togo_sock_get(object, &togo_sock TSRMLS_CC) < 0)
    {	
        RETURN_FALSE;
    }
    //param
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",&key, &key_len) == FAILURE)
    {
        RETURN_FALSE;
    }
    //cmd
    cmd_len = spprintf(&cmd, 0, "COUNTER GET %s\r\n", key);
    //sock write
    if (togo_sock_write(togo_sock, cmd, cmd_len TSRMLS_CC) < 0)
    {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    response = togo_sock_read(togo_sock, &response_len TSRMLS_CC);
    //sock read
    if (response == NULL)
    {
        RETURN_FALSE;
    }
    RETURN_LONG(atol(response));
}
/* }}} */

/* {{{ proto long Togo::counter_plus()
 */
PHP_METHOD(Togo, counter_plus)
{
	zval *object = getThis();
    TogoSock *togo_sock;
    char *cmd , *response ,*key = NULL;
    int cmd_len , response_len ,key_len;
    long value = 1 ;
    //togo sock get
    if (togo_sock_get(object, &togo_sock TSRMLS_CC) < 0)
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
    	cmd_len = spprintf(&cmd, 0, "COUNTER PLUS %s %d\r\n", key ,1);
    }else{
    	cmd_len = spprintf(&cmd, 0, "COUNTER PLUS %s %ld\r\n", key ,value);
    }
    //sock write
    if (togo_sock_write(togo_sock, cmd, cmd_len TSRMLS_CC) < 0)
    {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    response = togo_sock_read(togo_sock, &response_len TSRMLS_CC);
    //sock read
    if (response == NULL)
    {
        RETURN_FALSE;
    }
    RETURN_LONG(atol(response));
}
/* }}} */

/* {{{ proto long Togo::counter_reset()
 */
PHP_METHOD(Togo, counter_reset)
{
    zval *object = getThis();
    TogoSock *togo_sock;
    char *cmd , *response ,*key = NULL;
    int cmd_len , response_len ,key_len;
    long value = 1 ;
    //togo sock get
    if (togo_sock_get(object, &togo_sock TSRMLS_CC) < 0)
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
        cmd_len = spprintf(&cmd, 0, "COUNTER RESET %s %d\r\n", key ,1);
    }else{
        cmd_len = spprintf(&cmd, 0, "COUNTER RESET %s %ld\r\n", key ,value);
    }
    //sock write
    if (togo_sock_write(togo_sock, cmd, cmd_len TSRMLS_CC) < 0)
    {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    response = togo_sock_read(togo_sock, &response_len TSRMLS_CC);
    //sock read
    if (atol(response) == 0)
    {
        RETURN_TRUE;
    }
    RETURN_FALSE;
}
/* }}} */

/* {{{ proto long Togo::counter_minus()
 */
PHP_METHOD(Togo, counter_minus)
{
	zval *object = getThis();
    TogoSock *togo_sock;
    char *cmd , *response ,*key = NULL;
    int cmd_len , response_len ,key_len;
    long value = 1 ;
    //togo sock get
    if (togo_sock_get(object, &togo_sock TSRMLS_CC) < 0)
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
    	cmd_len = spprintf(&cmd, 0, "COUNTER MINUS %s %d\r\n", key ,1);
    }else{
    	cmd_len = spprintf(&cmd, 0, "COUNTER MINUS %s %ld\r\n", key ,value);
    }
    //sock write
    if (togo_sock_write(togo_sock, cmd, cmd_len TSRMLS_CC) < 0)
    {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    response = togo_sock_read(togo_sock, &response_len TSRMLS_CC);
    //sock read
    if (response == NULL)
    {
        RETURN_FALSE;
    }
    RETURN_LONG(atol(response));
}
/* }}} */

/* {{{ proto long Togo::lock_lock()
 */
PHP_METHOD(Togo, lock_lock)
{
    zval *object = getThis();
    TogoSock *togo_sock;
    char *cmd , *response ,*key = NULL;
    int cmd_len , response_len ,key_len;
    //togo sock get
    if (togo_sock_get(object, &togo_sock TSRMLS_CC) < 0)
    {   
        RETURN_FALSE;
    }
    //param
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",&key, &key_len) == FAILURE)
    {
        RETURN_FALSE;
    }
    //cmd
    cmd_len = spprintf(&cmd, 0, "LOCK LOCK %s\r\n", key);    
    //sock write
    if (togo_sock_write(togo_sock, cmd, cmd_len TSRMLS_CC) < 0)
    {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    response = togo_sock_read(togo_sock, &response_len TSRMLS_CC);
    //sock read
    if (atol(response) == 1)
    {
        RETURN_TRUE;
    }else{
        RETURN_FALSE;
    }
    
}
/* }}} */

/* {{{ proto long Togo::lock_unlock()
 */
PHP_METHOD(Togo, lock_unlock)
{
    zval *object = getThis();
    TogoSock *togo_sock;
    char *cmd , *response ,*key = NULL;
    int cmd_len , response_len ,key_len;
    //togo sock get
    if (togo_sock_get(object, &togo_sock TSRMLS_CC) < 0)
    {   
        RETURN_FALSE;
    }
    //param
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",&key, &key_len) == FAILURE)
    {
        RETURN_FALSE;
    }
    //cmd
    cmd_len = spprintf(&cmd, 0, "LOCK UNLOCK %s\r\n", key);    
    //sock write
    if (togo_sock_write(togo_sock, cmd, cmd_len TSRMLS_CC) < 0)
    {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    response = togo_sock_read(togo_sock, &response_len TSRMLS_CC);
    //sock read
    if (atol(response) == 0)
    {
        RETURN_TRUE;
    }else{
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ proto long Togo::lock_status()
 */
PHP_METHOD(Togo, lock_status)
{
    zval *object = getThis();
    TogoSock *togo_sock;
    char *cmd , *response ,*key = NULL;
    int cmd_len , response_len ,key_len;
    //togo sock get
    if (togo_sock_get(object, &togo_sock TSRMLS_CC) < 0)
    {   
        RETURN_FALSE;
    }
    //param
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",&key, &key_len) == FAILURE)
    {
        RETURN_FALSE;
    }
    //cmd
    cmd_len = spprintf(&cmd, 0, "LOCK STATUS %s\r\n", key);    
    //sock write
    if (togo_sock_write(togo_sock, cmd, cmd_len TSRMLS_CC) < 0)
    {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    response = togo_sock_read(togo_sock, &response_len TSRMLS_CC);
    //sock read
    RETURN_STRINGL(response, response_len, 0);
}
/* }}} */

/* {{{ proto bool Togo::queue_lpush()
 */
PHP_METHOD(Togo, queue_lpush)
{
    zval *object = getThis();
    TogoSock *togo_sock;
    char *cmd , *response ,*key = NULL;
    int cmd_len , response_len ,key_len;
    long value ,priority = 0;
    //togo sock get
    if (togo_sock_get(object, &togo_sock TSRMLS_CC) < 0)
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
        cmd_len = spprintf(&cmd, 0, "QUEUE LPUSH %s %ld %ld\r\n", key ,value ,priority);
    } else {
        cmd_len = spprintf(&cmd, 0, "QUEUE LPUSH %s %ld\r\n", key ,value);
    }
    //sock write
    if (togo_sock_write(togo_sock, cmd, cmd_len TSRMLS_CC) < 0)
    {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    response = togo_sock_read(togo_sock, &response_len TSRMLS_CC);
    //sock read
    if (response == NULL || strcmp(response ,TOGO_RESPONSE_STATUS_OK) != 0)
    {
        RETURN_FALSE;
    } else {
        RETURN_TRUE;
    }
}
/* }}} */

/* {{{ proto long Togo::queue_rpush()
 */
PHP_METHOD(Togo, queue_rpush)
{
    zval *object = getThis();
    TogoSock *togo_sock;
    char *cmd , *response ,*key = NULL;
    int cmd_len , response_len ,key_len;
    long value ,priority = 0;
    //togo sock get
    if (togo_sock_get(object, &togo_sock TSRMLS_CC) < 0)
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
        cmd_len = spprintf(&cmd, 0, "QUEUE RPUSH %s %ld %ld\r\n", key ,value ,priority);
    } else {
        cmd_len = spprintf(&cmd, 0, "QUEUE RPUSH %s %ld\r\n", key ,value);
    }
    //sock write
    if (togo_sock_write(togo_sock, cmd, cmd_len TSRMLS_CC) < 0)
    {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    response = togo_sock_read(togo_sock, &response_len TSRMLS_CC);
    //sock read
    if (strcmp(response ,TOGO_RESPONSE_STATUS_OK) == 0)
    {
        RETURN_TRUE;
    } else {
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ proto long Togo::queue_lpop()
 */
PHP_METHOD(Togo, queue_lpop)
{
    zval *object = getThis();
    TogoSock *togo_sock;
    char *cmd , *response ,*key = NULL;
    int cmd_len , response_len ,key_len;
    //togo sock get
    if (togo_sock_get(object, &togo_sock TSRMLS_CC) < 0)
    {   
        RETURN_FALSE;
    }
    //param
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",&key, &key_len) == FAILURE)
    {
        RETURN_FALSE;
    }
    //cmd
    cmd_len = spprintf(&cmd, 0, "QUEUE LPOP %s\r\n", key);
    //sock write
    if (togo_sock_write(togo_sock, cmd, cmd_len TSRMLS_CC) < 0)
    {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    response = togo_sock_read(togo_sock, &response_len TSRMLS_CC);
    //sock read
    if (response == NULL)
    {
        RETURN_FALSE;
    }
    RETURN_LONG(atol(response));
}
/* }}} */

/* {{{ proto long Togo::queue_rpop()
 */
PHP_METHOD(Togo, queue_rpop)
{
    zval *object = getThis();
    TogoSock *togo_sock;
    char *cmd , *response ,*key = NULL;
    int cmd_len , response_len ,key_len;
    //togo sock get
    if (togo_sock_get(object, &togo_sock TSRMLS_CC) < 0)
    {   
        RETURN_FALSE;
    }
    //param
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",&key, &key_len) == FAILURE)
    {
        RETURN_FALSE;
    }
    //cmd
    cmd_len = spprintf(&cmd, 0, "QUEUE RPOP %s\r\n", key);
    //sock write
    if (togo_sock_write(togo_sock, cmd, cmd_len TSRMLS_CC) < 0)
    {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    response = togo_sock_read(togo_sock, &response_len TSRMLS_CC);
    //sock read
    if (response == NULL)
    {
        RETURN_FALSE;
    }
    RETURN_LONG(atol(response));
}
/* }}} */

/* {{{ proto long Togo::queue_count()
 */
PHP_METHOD(Togo, queue_count)
{
    zval *object = getThis();
    TogoSock *togo_sock;
    char *cmd , *response ,*key = NULL;
    int cmd_len , response_len ,key_len;
    //togo sock get
    if (togo_sock_get(object, &togo_sock TSRMLS_CC) < 0)
    {   
        RETURN_FALSE;
    }
    //param
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",&key, &key_len) == FAILURE)
    {
        RETURN_FALSE;
    }
    //cmd
    cmd_len = spprintf(&cmd, 0, "QUEUE COUNT %s\r\n", key);
    //sock write
    if (togo_sock_write(togo_sock, cmd, cmd_len TSRMLS_CC) < 0)
    {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    response = togo_sock_read(togo_sock, &response_len TSRMLS_CC);
    //sock read
    if (response == NULL)
    {
        RETURN_FALSE;
    }
    RETURN_LONG(atol(response));
}
/* }}} */

/* {{{ proto string Togo::queue_status()
 */
PHP_METHOD(Togo, queue_status)
{
    zval *object = getThis();
    TogoSock *togo_sock;
    char *cmd , *response ,*key = NULL;
    int cmd_len , response_len ,key_len;
    //togo sock get
    if (togo_sock_get(object, &togo_sock TSRMLS_CC) < 0)
    {   
        RETURN_FALSE;
    }
    //param
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",&key, &key_len) == FAILURE)
    {
        RETURN_FALSE;
    }
    //cmd
    cmd_len = spprintf(&cmd, 0, "QUEUE STATUS %s\r\n", key);
    //sock write
    if (togo_sock_write(togo_sock, cmd, cmd_len TSRMLS_CC) < 0)
    {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    response = togo_sock_read(togo_sock, &response_len TSRMLS_CC);
    //sock read
    if (response == NULL)
    {
        RETURN_FALSE;
    }
    RETURN_STRINGL(response ,response_len ,0);
}
/* }}} */

/* {{{ proto boolean Togo::close()
 */
PHP_METHOD(Togo, close)
{
	zval *object = getThis();
	TogoSock *togo_sock;
    if (togo_sock_get(object, &togo_sock TSRMLS_CC) < 0)
    {
        RETURN_FALSE;
    }
    if (togo_sock_disconnect(togo_sock TSRMLS_CC))
    {
        RETURN_TRUE;
    }
    RETURN_FALSE;
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
