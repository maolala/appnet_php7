
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_appnet.h"
#include <stdio.h>

zval* appnet_tcpserv_callback[APPNET_TCP_SERVER_CALLBACK_NUM];
static int appnet_set_callback(int key, zval* cb TSRMLS_DC);
aeServer* serv;

//=====================================================
ZEND_METHOD( appTcpServer , __construct )
{
    size_t host_len = 0;
    char* serv_host;
    long  serv_port;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl", &serv_host, &host_len, &serv_port ) == FAILURE)
    {
        return;
    }
    aeServer* appserv = appnetTcpServInit( serv_host , serv_port );
    //save to global var
    appserv->ptr2 = getThis();
    APPNET_G( appserv ) = appserv;
}

ZEND_METHOD( appTcpServer , run )
{
   //serv->ptr2 = getThis();
   appnetTcpServRun();
}

ZEND_METHOD( appTcpServer , send )
{
   size_t len = 0;
   long fd;
   char* buffer;
   if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls", &fd,&buffer,&len ) == FAILURE)
   {
        return;
   }
   aeServer* appserv = APPNET_G( appserv );
   int sendlen; 
   sendlen = appserv->send( fd , buffer , len );
   RETURN_LONG( sendlen );
}


ZEND_METHOD( appTcpServer , close )
{
   long fd;
   if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &fd   ) == FAILURE)
   {
        RETURN_FALSE;;
   }

   if( fd <= 0 )
   {
      printf( "app close fd=%d error \n" , fd );
      RETURN_FALSE;
  }

   aeServer* appserv = APPNET_G( appserv );
   userClient* uc = &appserv->connlist[fd];
   if( uc == NULL )
   {
       php_printf( "close error,client obj is null");
       RETURN_FALSE;
   }
  // appserv->close( uc );
   //uc->close( fd );
   uc->flags = 5;
   php_printf( "close %d ok add=%ox" , fd , uc->close );
   RETURN_TRUE;
}



ZEND_METHOD( appTcpServer , on )
{
    size_t type_len,i,ret;
    char*  type;
    zval *cb;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz",  &type, &type_len, &cb ) == FAILURE)
    {
      return;
    }
    
    char *callback[APPNET_TCP_SERVER_CALLBACK_NUM] = {
        "connect",
        "receive",
        "close",
        "timer"
    };
    for (i = 0; i < APPNET_TCP_SERVER_CALLBACK_NUM; i++)
    {
        if (strncasecmp(callback[i], type , type_len ) == 0)
        {
            ret = appnet_set_callback(i, cb TSRMLS_CC);
            break;
        }
    }
    if (ret < 0)
    {
        php_error_docref(NULL TSRMLS_CC, E_ERROR, "Unknown event types[%s]", type );
    }
}

static int appnet_set_callback(int key, zval* cb TSRMLS_DC)
{
    appnet_tcpserv_callback[key] = emalloc(sizeof(zval));
    if (appnet_tcpserv_callback[key] == NULL)
    {
        return AE_ERR;
    }
    *(appnet_tcpserv_callback[key]) =  *cb;
    zval_copy_ctor( appnet_tcpserv_callback[key]);
    //printf( "callback.....%x\n" , &appnet_tcpserv_callback[key] );
    return AE_OK;
}


void appnetServerRecv( aeServer* s , userClient *c , int len )
{
     aeServer* appserv = APPNET_G( appserv );

//   int sendlen;
//   sendlen = appserv->send( c->fd , c->recv_buffer  , len );
//   printf( "sendlen=%d \n" , sendlen );

     zval retval;
     zval *args;
     zval *zserv = (zval*)appserv->ptr2;
     zval zdata;
     zval zfd;

     php_printf( "appnetServerRecv===%s len=%d flags=%d \n" , c->recv_buffer , len ,c->flags );
     args = safe_emalloc(sizeof(zval),3, 0 );

     ZVAL_LONG( &zfd , (long)c->fd );
     ZVAL_STRINGL( &zdata , c->recv_buffer, len );
     ZVAL_COPY(&args[0], zserv  );
     ZVAL_COPY(&args[1], &zfd  );
     ZVAL_COPY(&args[2], &zdata  );

     if (call_user_function_ex(EG(function_table), NULL, appnet_tcpserv_callback[APPNET_TCP_SERVER_CB_onReceive],&retval, 3, args, 0, NULL) == FAILURE )
     {
	 php_error_docref(NULL TSRMLS_CC, E_WARNING, "call_user_function_ex recv error");
     } 

     if (EG(exception))
     {
	php_error_docref(NULL, E_WARNING, "bind recv callback failed");
     }


     zval_ptr_dtor(&zfd);
     zval_ptr_dtor(&zdata);   

     //zval_ptr_dtor(&args[1]);
     //zval_ptr_dtor(&args[2]);
     efree( args );
     if ( &retval != NULL)
     {
        zval_ptr_dtor(&retval);
     }

}

void appnetServerClose( aeServer* s , userClient *c )
{
   aeServer* appserv = APPNET_G( appserv );
   zval retval;
   zval *args;
   zval *zserv = (zval*)appserv->ptr2;
   zval zfd;

   args = safe_emalloc(sizeof(zval),2, 0 );
   ZVAL_LONG( &zfd , (long)c->fd );

   ZVAL_COPY(&args[0], zserv  );
   ZVAL_COPY(&args[1], &zfd  );

   if (call_user_function_ex(EG(function_table), NULL, appnet_tcpserv_callback[APPNET_TCP_SERVER_CB_onClose],&retval, 2, args, 0, NULL) == FAILURE )
   {
      php_error_docref(NULL TSRMLS_CC, E_WARNING, "call_user_function_ex connect error");
   }

   if (EG(exception))
   {
      php_error_docref(NULL, E_WARNING, "bind accept callback failed");
   }
   zval_ptr_dtor(&zfd);
   efree( args );
   if ( &retval != NULL)
   {
     zval_ptr_dtor(&retval);
   } 
   s->close( c );
}




void appnetServerClose2( aeServer* s ,userClient *c )
{
    printf( "appnetServerClose fd=%d.....\n" , c->fd );
    zval retval;
    zval args[2];
    zval* zserv = (zval*)s->ptr2;
    args[0] = *zserv;
    ZVAL_LONG( &args[1] , c->fd );

    if (call_user_function_ex(EG(function_table), NULL, appnet_tcpserv_callback[APPNET_TCP_SERVER_CB_onClose] ,
  		 &retval, 2 , args, 0, NULL TSRMLS_CC) == SUCCESS )
    {
		zval_ptr_dtor(&retval);
    }else{     
		php_error_docref(NULL, E_WARNING, "bind close callback failed");
    }
    printf( "Worker Client closed  = %d  \n", c->fd );
    s->close( c );
}

void appnetServerConnect( aeServer* s ,int fd )
{
   printf( "New Client Connected fd =%d \n\n" , fd );
   aeServer* appserv = APPNET_G( appserv );
   zval retval;
   zval *args;
   zval *zserv = (zval*)appserv->ptr2;
   zval zfd;

   args = safe_emalloc(sizeof(zval),2, 0 );
   ZVAL_LONG( &zfd , (long)fd );

   ZVAL_COPY(&args[0], zserv  );
   ZVAL_COPY(&args[1], &zfd  );

   if (call_user_function_ex(EG(function_table), NULL, appnet_tcpserv_callback[APPNET_TCP_SERVER_CB_onConnect],&retval, 2, args, 0, NULL) == FAILURE )
   {
      php_error_docref(NULL TSRMLS_CC, E_WARNING, "call_user_function_ex connect error");
   }

   if (EG(exception))
   {
      php_error_docref(NULL, E_WARNING, "bind accept callback failed");
   }
   zval_ptr_dtor(&zfd);
   efree( args );
   if ( &retval != NULL)
   {
     zval_ptr_dtor(&retval);
   }

}

aeServer* appnetTcpServInit( char* listen_ip , int port  )
{
     serv = aeServerCreate( listen_ip , port );
     serv->onConnect = 	&appnetServerConnect;
     serv->onRecv = 	&appnetServerRecv;
     serv->onClose = 	&appnetServerClose;
     return serv;
}

void appnetTcpServRun()
{
     serv->runForever( serv );
}
