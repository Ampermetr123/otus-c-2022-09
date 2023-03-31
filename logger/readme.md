#  Tranparent On-Line Logger (TROLL)

- `troll` - library src 
- `examples` - examples how to use
- `test`  - some tests with Gtest framework


## What is TROLL about
 Troll embedds http server and memory storage to your programm 
 to keep recent log messages and show it  via browser.

## How to use

1. Link library and include `troll.h`
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

3. Start internal server:
```
troll_init("127.0.0.1:19999", 2048); 
```

`"127.0.0.1:19999"` - is troll server address
`2048` - is size of buffer, where stored logg messages


4. Now you can use printf-like function to log messages with
`troll_{category}` function (`category` is the name of defined category).

```
 troll_info("Hello world %d!", i);
```

5. Don't forget to stop serever and free resources when program is terminating
```
troll_release(); 
```

6. While your programm is running you can 
see all recent messages on-line via browser 
`http://127.0.0.1:19999`
