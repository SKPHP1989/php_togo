<?php
try {
	$togo = new Togo;
	$togo->connect('127.0.0.1' ,8787 ,1);
	var_dump('$togo->version()' ,$togo->version());
	var_dump('$togo->counter_plus(testtt)' ,$togo->counter_plus('testtt' ,4));
	var_dump('$togo->counter_get(testtt)' ,$togo->counter_get('testtt'));
	var_dump('$togo->counter_minus(testtt)' ,$togo->counter_minus('testtt'));
	var_dump('$togo->counter_get(testtt)' ,$togo->counter_get('testtt'));
	var_dump('$togo->lock_lock(testtt)' ,$togo->lock_lock('testtt'));
	var_dump('$togo->lock_unlock(testtt)' ,$togo->lock_unlock('testtt'));
	var_dump('$togo->lock_status(testtt)' ,$togo->lock_status('testtt'));
	var_dump('$togo->queue_lpush(queue)' ,$togo->queue_lpush('queue' ,1));
	var_dump('$togo->queue_lpush(queue)' ,$togo->queue_lpush('queue' ,2));
	var_dump('$togo->queue_lpop(queue)' ,$togo->queue_lpop('queue'));
	var_dump('$togo->queue_lpop(queue)' ,$togo->queue_lpop('queue'));
	var_dump('$togo->queue_lpop(queuel)' ,$togo->queue_rpush('queuel' ,1));
	var_dump('$togo->queue_lpop(queuel)' ,$togo->queue_rpush('queuel' ,2));
	var_dump('$togo->queue_count(queuel)' ,$togo->queue_count('queuel'));
	var_dump('$togo->queue_status(queuel)' ,$togo->queue_status('queuel'));
	var_dump('$togo->close()' ,$togo->close('testtt'));
}catch (TogoException $e){
	var_dump($e);
}