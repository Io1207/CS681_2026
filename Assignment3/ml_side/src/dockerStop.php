<html>
<body>
<?php
    $command = "sudo docker stop web_server && sudo docker rm web_server";
    system ($command);
?>
</html>
</body>
