#PHP EXTENSION FOR TOGO

#环境要求
Linux,php5.4-5.6<br/>
(ps:本人亲测,5.4,5.5,5.6版本是可以的!)<br/>

#安装php_togo
root@localhost#wget https://raw.githubusercontent.com/SKPHP1989/php_togo/master/version/php_togo_0.0.1.zip<br/>
root@localhost#unzip master.zip<br/>
root@localhost#cd master<br/>
root@localhost#php-src/bin/phpize --with-php-config=php-src/bin/php-config<br/>
root@localhost#./configure<br/>
root@localhost#make && make install<br/>
<br/>
修改php.ini文件 加载togo.so扩展库<br/>
重启apache或者php-fpm<br/>
<br/>
检测是否已经加载togo<br/>
root@localhost#php-src/bin/php -m|grep togo<br/>
出现togo说明已经加载<br/>

##togo地址:https://github.com/zhuli/togo


```

功能：
1. 初始化以及常用说明
2. 计数器函数
3. 队列函数
4. 内存锁函数

#1.初始化以及常用说明
try{
	$togo = new Togo;
	$togo->connect('127.0.0.1' ,8787 ,5);
	$togo->close();
}catch(TogoException $e){
	var_dump($e)
}
```

##1 连接togo
### connect

##### *Paramer*

*host*: string.<br>
*port*: int.<br>
*timeout*:int.

##### *Return value*

*BOOL*: `TRUE` on success, `FALSE` on error.

##### *Example*

~~~
$togo->connect('127.0.0.1' ,8787 ,5);
~~~

##2 获取togo版本
### version

##### *Paramer*

##### *Return value*

*BOOL*: `STRING` on success, `FALSE` on error.

##### *Example*

~~~
$togo->version();
~~~

##3 关闭togo连接
### close

##### *Paramer*

##### *Return value*

*BOOL*: `TRUE` on success, `FALSE` on error.

##### *Example*

~~~
$togo->close();
~~~

#2.计数器函数：

##1 计数器加上某个数字，默认+1
### counter_plus

##### *Paramer*

*name*: string.<br>
*value*: int, optional ,default 1.

##### *Return value*

*BOOL*: `INT` on success, `FALSE` on error.

##### *Example*

~~~
$togo->counter_plus('test');
$togo->counter_plus('test' ,6);
~~~

##2 计数器减去某个数字，默认-1
### counter_minus

##### *Paramer*

*name*: string. <br>
*value*: int, optional ,default 1.

##### *Return value*

*BOOL*: `INT` on success, `FALSE` on error.

##### *Example*

~~~
$togo->counter_minus('test');
$togo->counter_minus('test' ,6);
~~~

##3 计数器获取一个值
### counter_get

##### *Paramer*

*name*: string.

##### *Return value*

*BOOL*: `INT` on success, `FALSE` on error.

##### *Example*

~~~
$togo->counter_get('test');
~~~

##4 计数器初始化
### counter_reset

##### *Paramer*

*name*: string.

##### *Return value*

*BOOL*: `TRUE` on success, `FALSE` on error.

##### *Example*

~~~
$togo->counter_reset('test');
~~~

#3.队列消息函数：

##1 从左边插入一个记录
### queue_lpush

##### *Paramer*

*name*: string.<br>
*value*: int.<br>
*priority*: int, optional.

##### *Return value*

*BOOL*: `TRUE` on success, `FALSE` on error.

##### *Example*

~~~
$togo->queue_lpush('test_queue' ,1234);
$togo->queue_lpush('test_queue' ,1234 ,2);
~~~

##2 从右边插入一个记录
### queue_rpush

##### *Paramer*

*name*: string.<br>
*value*: int.<br>
*priority*: int, optional.

##### *Return value*

*BOOL*: `TRUE` on success, `FALSE` on error.

##### *Example*

~~~
$togo->queue_rpush('test_queue' ,1234);
$togo->queue_rpush('test_queue' ,1234 ,2);
~~~

##3 从左边获取一个记录
### queue_lpop

##### *Paramer*

*name*: string.

##### *Return value*

*BOOL*: `INT` on success, `FALSE` on error.

##### *Example*

~~~
$togo->queue_lpop('test_queue' );
~~~

##4 从右边获取一个记录
### queue_rpop

##### *Paramer*

*name*: string.

##### *Return value*

*BOOL*: `INT` on success, `FALSE` on error.

##### *Example*

~~~
$togo->queue_rpop('test_queue' );
~~~

##5 获取一个队列的总记录数
### queue_count

##### *Paramer*

*name*: string.

##### *Return value*

*BOOL*: `INT` on success, `FALSE` on error.

##### *Example*

~~~
$togo->queue_count('test_queue' );
~~~

##6 获取一个队列的状态
### queue_status

##### *Paramer*

*name*: string.

##### *Return value*

*BOOL*: `STRING` on success, `FALSE` on error.

##### *Example*

~~~
$togo->queue_status('test_queue' );
~~~

#4.内存锁消息函数：
##1 LOCK操作
### lock_lock

##### *Paramer*

*name*: string.

##### *Return value*

*BOOL*: `TRUE` on success, `FALSE` on error.

##### *Example*

~~~
$togo->lock_lock('test_lock' );
~~~

##2 UNLOCK操作
### lock_unlock

##### *Paramer*

*name*: string.

##### *Return value*

*BOOL*: `TRUE` on success, `FALSE` on error.

##### *Example*

~~~
$togo->lock_unlock('test_lock' );
~~~

##3 获取一把锁的状态
### lock_status

##### *Paramer*

*name*: string.

##### *Return value*

*BOOL*: `STRING` on success, `FALSE` on error.

##### *Example*

~~~
$togo->lock_status('test_lock' );
~~~
