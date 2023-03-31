#pragma once

static const char* html_index= "<!DOCTYPE html>\n\
<html>\n\
<head>\n\
  <meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">\n\
  <title>Troll logger viewer</title>\n\
</head>\n\
<body>\n\
  <script type=\"text/javascript\">\n\
    function add_message_to_table(msg) {\n\
      table = document.getElementById(\"msg_table\");\n\
      var tr = document.createElement(\"tr\"),\n\
        tdMsg = document.createElement(\"td\");\n\
      tdMsg.textContent = msg;\n\
      tr.appendChild(tdMsg);\n\
      table.appendChild(tr);\n\
    }\n\n\
    window.addEventListener(\"DOMContentLoaded\", function() {\n\
      const evtSource = new EventSource(\"events\");\n\
      evtSource.onmessage = (event) => {\n\
        add_message_to_table(`${event.data}`);\n\
      }\n\
    })        \n\
  </script>\n\
\n\
  <table>\n\
    <thead>\n\
      <tr>\n\
        <th>Messages</th>\n\
      </tr>\n\
    </thead>\n\
    <tbody id=\"msg_table\">\n\
    </tbody>\n\
  </table>\n\
</body>\n\
</html>\n";
