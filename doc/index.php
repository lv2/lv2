<?php
# Content-type negotiation for LV2 specification bundles

$rdfxml      = accepts("application\/rdf\+xml");
$turtle      = accepts("application\/turtle");
$x_turtle    = accepts("application\/x-turtle");
$text_turtle = accepts("text\/turtle");
$json        = accepts("application\/json");
$html        = accepts("text\/html");
$xhtml       = accepts("application\/xhtml\+xml");
$text_plain  = accepts("text\/plain");

$name = basename($_SERVER['REQUEST_URI']);

# Return Turtle ontology
if ($turtle or $x_turtle or $text_turtle) {
	header("Content-Type: application/x-turtle");
	passthru("cat ./$name.ttl");

# Return ontology translated into rdf+xml
} else if ($rdfxml) {
	header("Content-Type: application/rdf+xml");
	passthru("~/bin/rapper -q -i turtle -o rdfxml-abbrev ./$name.ttl");

} else if ($json) {
	header("Content-Type: application/json");
	passthru("~/bin/rapper -q -i turtle -o json ./$name.ttl");

# Return HTML documentation
} else if ($html or $xhtml) {
	if ($html) {
		header("Content-Type: text/html");
	} else {
		header("Content-Type: application/xhtml+xml");
	}
	$name = basename($_SERVER['REQUEST_URI']); 
	passthru("cat ./$name.html | sed '
s/<\/body>/<div style=\"font-size: smaller; color: gray; text-align: right; margin: 1ex;\">This document is content-negotiated.  If you request it with <code>Accept: application\/x-turtle<\/code> you will get the description in Turtle.  Also supported: <code>application\/rdf+xml<\/code>, <code>application\/json<\/code>, <code>text\/plain<\/code><\/div><\/body>/'");

# Return NTriples (text/plain)
} else if ($text_plain) {
	header("Content-Type: text/plain");
	passthru("~/bin/rapper -q -i turtle -o ntriples ./$name.ttl");

# Return Turtle ontology by default
} else {
	header("Content-Type: application/x-turtle");
	passthru("cat ./$name.ttl");
}

function accepts($type) {
	global $_SERVER;
	if (preg_match("/$type(;q=(\d+\.\d+))?/i", $_SERVER['HTTP_ACCEPT'], $matches)) {
		return isset($matches[2]) ? $matches[2] : 1;
	} else {
		return 0;
	}
}

?>
