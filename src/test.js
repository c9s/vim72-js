
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
ok( buf_ffname(b) );

message( buf_number( b ) );

//is( buf_ffname(b) , '/Users/c9s/git-working/vim7/src/test.js' , 'buf_ffname: ' + buf_ffname(b)  );


/* 
if( name ) 
    message( name );
else
    message( 'buf_ffname not ok' );

    */
