## 快速入门

Melon的使用并不繁琐，大致步骤可分为：

1. 在使用前初始化库
2. 引入对应功能头文件，调用其中函数进行使用
3. 启动程序前根据需要修改配置文件

#### 组件使用示例

下面看一个使用内存池的例子：

```c
#include <stdio.h>
#include "mln_core.h"
#include "mln_alloc.h"
#include <mln_log.h>

int main(int argc, char *argv[])
{
    char *ptr;
    mln_alloc_t *pool;
    struct mln_core_attr cattr;

    cattr.argc = argc;
    cattr.argv = argv;
    cattr.global_init = NULL;
    cattr.master_process = NULL;
    cattr.worker_process = NULL;

    if (mln_core_init(&cattr) < 0) {
       fprintf(stderr, "Melon init failed.\n");
       return -1;
    }

    pool = mln_alloc_init(NULL);

    ptr = mln_alloc_m(pool, 1024);
    mln_log(debug, "%X\n", ptr);
    mln_alloc_free(ptr);

    mln_alloc_destroy(pool);
    return 0;
}
```

其中，`mln_core_init`就是Melon库的初始化函数，函数参数是一个结构体，该结构体用于传入`程序参数`、`全局变量初始化函数`以及`工作进程处理函数`。由于这个例子中并不打算启用多进程框架，也不需要初始化一些全局变量，因此两个函数指针都被置空了。

随后的代码中：

- `mln_alloc_init`：用于创建内存池
- `mln_alloc_m`： 用于从内存池中分配一块指定大小的内存
- `mln_alloc_free`：用于将内存池分配的内存释放回池中
- `mln_alloc_destroy`：用于销毁内存池并释放资源

内存池相关的函数及结构定义都在`mln_alloc.h`中。

`mln_log`为Melon的日志输出函数，本例中以十六进制输出内存池分配的内存起始地址。



随后对代码进行编译，这里以UNIX系统为例：

```bash
$ cc -o test test.c -I /usr/local/melon/include/ -L /usr/local/melon/lib/ -lmelon
```

Windows用户也可以在git bash中执行：

```bash
$ gcc -o test test.c -I $HOME/libmelon/include/ -L $HOME/libmelon/lib/ -llibmelon -lWs2_32
```



此时，还不可以启动这个`test`程序，因为我们先要检查Melon库的配置文件，确保配置不会使得程序启动多进程或者多线程框架（Windows用户可以忽略此步骤）。

```
$ vim /usr/local/melon/conf/melon.conf

log_level "none";
//user "root";
daemon off;
core_file_size "unlimited";
//max_nofile 1024;
worker_proc 1;
thread_mode off;
framework off;
log_path "/usr/local/melon/logs/melon.log";
/*
 * Configurations in the 'exec_proc' are the
 * processes which are customized by user.
 *
 * Here is an example to show you how to
 * spawn a program.
 *     keepalive "/tmp/a.out" ["arg1" "arg2" ...]
 * The command in this example is 'keepalive' that
 * indicate master process to supervise this
 * process. If process is killed, master process
 * would restart this program.
 * If you don't want master to restart it, you can
 *     default "/tmp/a.out" ["arg1" "arg2" ...]
 *
 * But you should know that there is another
 * arugment after the last argument you write here.
 * That is the file descriptor which is used to
 * communicate with master process.
 */
exec_proc {
   // keepalive "/tmp/a";
}
thread_exec {
//    restart "hello" "hello" "world";
//    default "haha";
}
```

这里我们需要确保`framework`这一项为`off`，因为本例不需要启动框架功能。



此时，我们就可以执行这个例子了。

```bash
$ ./test
```

此时可以看到类似如下输出内容：

```
03/27/2021 04:36:26 GMT DEBUG: test.c:main:25: PID:24077 1e29950
```



#### 多进程框架使用示例

框架功能在Windows中，暂时不支持，本例将在UNIX系统中进行演示。

```c
#include <stdio.h>
#include "mln_core.h"
#include "mln_log.h"
#include "mln_event.h"

char text[1024];

static int global_init(void);
static void worker_process(mln_event_t *ev);
static void print_handler(mln_event_t *ev, void *data);

int main(int argc, char *argv[])
{
    struct mln_core_attr cattr;
    cattr.argc = argc;
    cattr.argv = argv;
    cattr.global_init = global_init;
    cattr.master_process = NULL;
    cattr.worker_process = worker_process;
    return mln_core_init(&cattr);
}

static int global_init(void)
{
    //global variable init function
    int n = snprintf(text, sizeof(text)-1, "hello world\n");
    text[n] = 0;
    return 0;
}

static void worker_process(mln_event_t *ev)
{
    //we can set event handler here
    //let's set a timer
    mln_event_timer_set(ev, 1000, text, print_handler);
}

static void print_handler(mln_event_t *ev, void *data)
{
    mln_log(debug, "%s\n", (char *)data);
    mln_event_timer_set(ev, 1000, data, print_handler);
}
```

在本例中，我们增加了`global_init`和`worker_process`的处理函数。`global_init`用于初始化一个全局的字符数组`text`。而`worker_process`则是子进程（或称为工作进程）的处理函数。在工作进程处理函数中，我们使用到了Melon事件模块的定时器函数，用于每1秒中（1000毫秒），调用一次`print_handler`函数将字符数组`text`中的内容进行日志输出。

生成可执行程序：

```bash
$ cc -o hello hello.c -I /usr/local/melon/include/ -L /usr/local/melon/lib/ -lmelon
```

接着，依旧是检查配置文件，但这一次我们要确保`framework`须为`on`，`thread_mode`为`off`。这样的配置表明，我们启用Melon的框架功能，但不启用多线程模式，那么Melon就会启用多进程模式。然后，根据需要修改`worker_proc`的数量，例如：3。

```
log_level "none";
//user "root";
daemon off;
core_file_size "unlimited";
//max_nofile 1024;
worker_proc 3;
thread_mode off;
framework on;
log_path "/usr/local/melon/logs/melon.log";
/*
 * Configurations in the 'exec_proc' are the
 * processes which are customized by user.
 *
 * Here is an example to show you how to
 * spawn a program.
 *     keepalive "/tmp/a.out" ["arg1" "arg2" ...]
 * The command in this example is 'keepalive' that
 * indicate master process to supervise this
 * process. If process is killed, master process
 * would restart this program.
 * If you don't want master to restart it, you can
 *     default "/tmp/a.out" ["arg1" "arg2" ...]
 *
 * But you should know that there is another
 * arugment after the last argument you write here.
 * That is the file descriptor which is used to
 * communicate with master process.
 */
exec_proc {
   // keepalive "/tmp/a";
}
thread_exec {
//    restart "hello" "hello" "world";
//    default "haha";
}
```

此时，我们可以启动程序了：

```bash
$ ./hello
```

我们可以看到类似如下的输出：

```
Start up worker process No.1
Start up worker process No.2
Start up worker process No.3
03/27/2021 04:53:44 GMT DEBUG: d.c:print_handler:39: PID:27620 hello world

03/27/2021 04:53:44 GMT DEBUG: d.c:print_handler:39: PID:27621 hello world

03/27/2021 04:53:44 GMT DEBUG: d.c:print_handler:39: PID:27622 hello world

03/27/2021 04:53:45 GMT DEBUG: d.c:print_handler:39: PID:27620 hello world

03/27/2021 04:53:45 GMT DEBUG: d.c:print_handler:39: PID:27621 hello world

03/27/2021 04:53:45 GMT DEBUG: d.c:print_handler:39: PID:27622 hello world

...
```



到此，快速入门部分就告一段落了，下面我们会针对Melon所提供的每一个功能进行说明并给予示例。
