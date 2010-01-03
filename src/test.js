
//system('ls');
//message('orz');   // show vim message
//alert('orz');   // show vim message  (alias to message function)

function ok(v,comment) {
    if( v ) 
	message( "ok - " + comment );
    else
	message( "fail - " + comment );
}

/* 
function is(v,t,comment) {
    //arguments.callee.caller.toString()
    if( v == t ) 
	message( "ok - " + comment );
    else
	message( "fail - " + comment );
}

*/

var i = buf_cnt();
message( i );
ok( i , 'buf_cnt' );
ok( buf_cnt() , 'buf_cnt');

var b = buf_nr(1);
ok( b , 'buf_nr' );

message( { a: 123 } );
message( "String" );
message( function() {  return 123;   } );
message( b );
message( b.number() );
message( b.ffname() );
message( b.sfname() );
/*
message( b.fname() );
message( b.line(2) );
message( b.next() );
if ( b.next() )
    message( b.next().fname() );
message( b.prev() );
message( b.lines(1,10) );


var lines = b.lines(1,10);
for ( i in lines ) {
    message( i + ":" + lines[i] );
}

*/
