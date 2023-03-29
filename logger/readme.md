#  Tranparent On-Line Logger (TROLL)

`troll` - library src 
`examples` - examples how to use
`test`  - some tests with Gtest framework


## What is TROLL about
 Troll logger embeds hhtp server and memory circular storage
 to keep and show recent log messages on line 
 via http protocol.


## How to use

1. include `troll.h`
```
#inlcude "troll.h"
```
2. define logger levels (or categories) you need
```
#define LOG_LEVEL_FATAL 1
#define TROLL_LEVEL_MASK LOG_LEVEL_FATAL
#define TROLL_LEVEL_NAME fatal
#define TROLL_LEVEL_FORMAT 
#include "troll_reg_level.h"

#define LOG_LEVEL_INFO 2
#define TROLL_LEVEL_MASK LOG_LEVEL_INFO
#define TROLL_LEVEL_NAME info
#define TROLL_LEVEL_FORMAT
#include "troll_reg_level.h"
```

After that you can use printf-like
`troll_{category}` functionm where `category` is the name of defined category.


3. Start internal server before use
```
troll_init("127.0.0.1:19999", 2048); 
```
`"127.0.0.1:19999"` - is troll server address
`2048` - is size of buffer, where stored logg messages

4. use it
```
 troll_info("Hello world %d!", i);
```

5. don't forget to release when program terminates
```
troll_release(); 
```

6. while you programm is running you can 
see all recent messages on-line via browser 
`http://127.0.0.1:19999`
