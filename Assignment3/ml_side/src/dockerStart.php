<html>
<body>
<?php
    $url =  "//{$_SERVER['HTTP_HOST']}{$_SERVER['REQUEST_URI']}";
    $escaped_url = htmlspecialchars( $url, ENT_QUOTES, 'UTF-8' );
    $url_components = parse_url($url);
    parse_str($url_components['query'], $params);
    $command = "sudo docker run --privileged -m {$params['memory']} --memory-swap 1024m --cpus {$params['cpus']} --device-write-bps /dev/sda:{$params['device-write']} --name web_server -d -p 80:80 web_server:1.0";
    $output = system($command, $ret);
?>
</body>
</html>
