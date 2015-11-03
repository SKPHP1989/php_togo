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

#ifndef PHP_TOGO_H
#define PHP_TOGO_H

extern zend_module_entry togo_module_entry;
#define phpext_togo_ptr &togo_module_entry

#define PHP_TOGO_VERSION "0.1.0" /* Replace with version number for your extension */

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

//togo method
PHP_METHOD(Togo, __construct);
PHP_METHOD(Togo, __destruct);
PHP_METHOD(Togo, connect);
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

//togo ini
PHP_MINIT_FUNCTION(togo);
PHP_MSHUTDOWN_FUNCTION(togo);
PHP_RINIT_FUNCTION(togo);
PHP_RSHUTDOWN_FUNCTION(togo);
PHP_MINFO_FUNCTION(togo);

/* {{{ struct togo_sock */
typedef struct togo_sock_ {
    php_stream     *stream;
    char           *host;
    unsigned short port;
    long           timeout;
    int            status;
} TogoSock;
/* }}} */

#define togo_sock_name "Togo Socket Buffer"

#define TOGO_SOCK_STATUS_FAILED 0
#define TOGO_SOCK_STATUS_DISCONNECTED 1
#define TOGO_SOCK_STATUS_UNKNOWN 2
#define TOGO_SOCK_STATUS_CONNECTED 3

/* properties */
#define TOGO_NOT_FOUND 0
#define TOGO_STRING 1
#define TOGO_SET 2
#define TOGO_LIST 3

/* togo reponse message header content */
#define TOGO_RESPONE_HEADER "TOGO_S"
/* togo reponse message tail content */
#define TOGO_RESPONE_TAIL "TOGO_E"

/* togo reponse message status */
#define TOGO_RESPONSE_STATUS_OK "TOGO_OK"

#define TOGO_RESPONSE_STATUS_FAIL "TOGO_FAIL"
#define TOGO_RESPONSE_STATUS_NULL "TOGO_NULL"
#define TOGO_RESPONSE_STATUS_COMMANDBIG "TOGO_COMMAND_TOO_BIG"
#define TOGO_RESPONSE_STATUS_BIG "TOO_BIG"
#define TOGO_RESPONSE_STATUS_IS_EXIST "TOGO_IS_EXIST"
#define TOGO_RESPONSE_STATUS_NOT_EXIST "TOGO_NOT_EXIST"

/* togo reponse message code */
#define TOGO_RESPONSE_STATUS_OK_CODE 0x00000000

#define TOGO_RESPONSE_CODE_FAIL 0x00010001
#define TOGO_RESPONSE_CODE_NULL 0x00010002
#define TOGO_RESPONSE_CODE_COMMANDBIG 0x00010003
#define TOGO_RESPONSE_CODE_BIG_CODE 0x00010004
#define TOGO_RESPONSE_CODE_IS_EXIST 0x00010005
#define TOGO_RESPONSE_CODE_NOT_EXIST 0x00010006
#define TOGO_RESPONSE_CODE_ERROR 0x00010007
#define TOGO_RESPONSE_CODE_CONNECT_FAIL 0x00010008
#define TOGO_RESPONSE_CODE_CONNECT_TIMEOUT 0x00010009

/* togo reponse message content */
#define TOGO_RESPONSE_MSG_FAIL "Togo fail to handle request!"
#define TOGO_RESPONSE_MSG_NULL "Togo response is null!"
#define TOGO_RESPONSE_MSG_COMMANDBIG "Togo Commond is too big!"
#define TOGO_RESPONSE_MSG_BIG "Togo get or send message is too big!"
#define TOGO_RESPONSE_MSG_IS_EXIST "Togo element is exists!"
#define TOGO_RESPONSE_MSG_NOT_EXIST "Togo element is not exists!"
#define TOGO_RESPONSE_MSG_ERROR "Togo protocol error!"
#define TOGO_RESPONSE_MSG_CONNECT_FAIL "Togo can't connect to %s:%d"
#define TOGO_RESPONSE_MSG_CONNECT_TIMEOUT "Togo connect host %s:%u over %u second!"

/* {{{ internal function protos */
void add_constant_long(zend_class_entry *ce, char *name, int value);

PHPAPI void togo_check_eof(TogoSock *togo_sock TSRMLS_DC);
PHPAPI TogoSock* togo_sock_create(char *host, int host_len, unsigned short port, long timeout);
PHPAPI int togo_sock_connect(TogoSock *togo_sock TSRMLS_DC);
PHPAPI int togo_sock_disconnect(TogoSock *togo_sock TSRMLS_DC);

PHPAPI int togo_sock_server_open(TogoSock *togo_sock, int TSRMLS_DC);
PHPAPI void togo_free_socket(TogoSock *togo_sock);
PHPAPI int togo_sock_get(zval *id, TogoSock **togo_sock TSRMLS_DC);
PHPAPI char * togo_sock_read(TogoSock *togo_sock, int *buf_len TSRMLS_DC);
PHPAPI int togo_sock_write(TogoSock *togo_sock, char *cmd, size_t sz TSRMLS_DC);
PHPAPI int togo_sock_response_status_check(char *inbuf TSRMLS_DC);
PHPAPI char * togo_sock_response_parser(char *inbuf TSRMLS_DC);
/* }}} */


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
