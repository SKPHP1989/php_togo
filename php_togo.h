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
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef PHP_TOGO_H
#define PHP_TOGO_H

extern zend_module_entry togo_module_entry;
#define phpext_togo_ptr &togo_module_entry

#define PHP_TOGO_VERSION "1.0.1"
#define PHP_TOGO_AUTHOR	"Michael Lee"
#ifdef PHP_WIN32
#	define PHP_TOGO_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_TOGO_API __attribute__ ((visibility("default")))
#else
#	define PHP_TOGO_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#define CMD_END_SYMBOL	"\r\n"
#define RESPONSE_END_SYMBOL	"\r\n"

#ifndef NULL
#define	NULL	0
#endif

typedef struct togo_sock_ {
    php_stream     *stream;
    char           *host;
    unsigned short port;
    long           timeout;
    int            state;
} TogoSock;

#define TOGO_SOCK_STATE_FAILED 0
#define TOGO_SOCK_STATE_DISCONNECTED 1
#define TOGO_SOCK_STATE_UNKONW 2
#define TOGO_SOCK_STATE_CONNECTED 3

#define TOGO_SOCK_DEFAULT_HOST "127.0.0.1"
#define TOGO_SOCK_DEFAULT_PORT 6377
#define TOGO_SOCK_DEFAULT_TIMEOUT 5

#define TOGO_EXCEPTION_CODE_PARAM 0x01001
#define TOGO_EXCEPTION_CODE_TIMEOUT 0x01002
#define TOGO_EXCEPTION_CODE_CONNECT_FAIL 0x01003
#define TOGO_EXCEPTION_CODE_PROTOCOL_ERR 0x01004
#define TOGO_EXCEPTION_CODE_CONN_ERR 0x01005

#define TOGO_EXCEPTION_CODE_FAIL 0x020001
#define TOGO_EXCEPTION_CODE_NULL 0x020002
#define TOGO_EXCEPTION_CODE_COMMANDBIG 0x020003
#define TOGO_EXCEPTION_CODE_BIG_CODE 0x020004
#define TOGO_EXCEPTION_CODE_IS_EXIST 0x020005
#define TOGO_EXCEPTION_CODE_NOT_EXIST 0x020006

#define TOGO_EXCEPTION_MSG_PARAM "(%s)Paramers error,please check!"
#define TOGO_EXCEPTION_MSG_TIMEOUT "(%s)Timeout %l value is invalid!"
#define TOGO_EXCEPTION_MSG_CONNECT_FAIL "(%s)Connect %s:%ld(%ld) failed or timeout!"
#define TOGO_EXCEPTION_MSG_PROTOCOL_ERR "Protocol error!"
#define TOGO_EXCEPTION_MSG_CONN_ERR "(%s)Connect server error or broken!"

#define TOGO_RESPONSE_MSG_OK "TOGO_OK"
#define TOGO_RESPONSE_MSG_FAIL "TOGO_FAIL"
#define TOGO_RESPONSE_MSG_NULL "TOGO_NULL"
#define TOGO_RESPONSE_MSG_COMMANDBIG "TOGO_COMMAND_TOO_BIG"
#define TOGO_RESPONSE_MSG_BIG "TOO_BIG"
#define TOGO_RESPONSE_MSG_IS_EXIST "TOGO_IS_EXIST"
#define TOGO_RESPONSE_MSG_NOT_EXIST "TOGO_NOT_EXIST"

#define TOGO_RESPONSE_MSG_HEADER "TOGO_S"
#define TOGO_RESPONSE_MSG_END "TOGO_E"
#define TOGO_SOCK_NAME "Togo socket buffer"

static void togo_destructor_togo_sock(zend_rsrc_list_entry * TSRMLS_DC);
PHPAPI zend_class_entry *togo_get_exception_base(int TSRMLS_DC);
PHPAPI void togo_sock_free(TogoSock *);
PHPAPI zend_class_entry *togo_get_exception_base(int TSRMLS_CC);
PHPAPI TogoSock* togo_sock_create(char *,int  ,unsigned short ,long);
PHPAPI int togo_sock_connect(TogoSock * TSRMLS_DC);
PHPAPI int toso_sock_open_server(TogoSock *,int TSRMLS_DC);
PHPAPI int togo_sock_disconnect(TogoSock * TSRMLS_DC);
PHPAPI int togo_sock_write(TogoSock * ,char * ,size_t TSRMLS_DC);
PHPAPI void togo_sock_eof(TogoSock * TSRMLS_DC);
PHPAPI int togo_sock_get(TogoSock ** ,zval * TSRMLS_DC);
PHPAPI char *togo_sock_read(TogoSock *, int * TSRMLS_DC);
PHPAPI char *togo_sock_response_parser(char *inbuf TSRMLS_DC);
PHPAPI int togo_sock_close(zval *);
PHPAPI int togo_sock_response_state_check(char * TSRMLS_DC);

PHP_METHOD(Togo ,__construct);
PHP_METHOD(Togo ,__destruct);
PHP_METHOD(Togo ,connect);
PHP_METHOD(Togo ,write);
PHP_METHOD(Togo, version);
PHP_METHOD(Togo, counter_plus);
PHP_METHOD(Togo, counter_reset);
PHP_METHOD(Togo, counter_minus);
PHP_METHOD(Togo, counter_get);
PHP_METHOD(Togo, lock_lock);
PHP_METHOD(Togo, lock_unlock);
PHP_METHOD(Togo, lock_status);
PHP_METHOD(Togo, queue_lpush);
PHP_METHOD(Togo, queue_lpop);
PHP_METHOD(Togo, queue_rpush);
PHP_METHOD(Togo, queue_rpop);
PHP_METHOD(Togo, queue_count);
PHP_METHOD(Togo, queue_status);
PHP_METHOD(Togo, close);

#ifdef ZTS
#define TOGO_G(v) TSRMG(togo_globals_id, zend_togo_globals *, v)
#else
#define TOGO_G(v) (togo_globals.v)
#endif

#endif	/* PHP_TOGO_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
