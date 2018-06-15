#include <linux/fs.h>               // open(), read(), write(), close()
#include <linux/cdev.h>             // register_chardev_region(), cdev_init)()
#include <linux/module.h>           
#include <linux/io.h>               // ioremap(), iounmap()
#include <linux/uaccess.h>          // copy_from_user(), copy_to_user()
#include <linux/gpio.h>             // request_gpio(), gpio_set_val(), gpio_get_val()
#include <linux/interrupt.h>        // gpio_to_irq(), request_irq()
#include <linux/timer.h>            // init_timer(), mod_timer(), del_timer(), add_timer()
//#include <asm/siginfo.h>            // siginfo 구조체를 사용하기 위해
#include <linux/sched/signal.h>

#define GPIO_MAJOR 200
#define GPIO_MINOR 0
#define GPIO_DEVICE "gpioled"
/*
//Raspi 0, 1 PHYSICAL I/O PERI BASE ADDR
#define BCM_IO_BASE 0x20000000
*/

//Raspi 3 PHYSICAL I/O PERI BASE ADDR
#define BCM_IO_BASE 0x3F000000

//GPIO ADDR(BASE_ADDR + 0x200000)
#define GPIO_BASE (BCM_IO_BASE + 0x200000)

//#define GPIO_SIZE 0xB4

/*
#define GPIO_IN(g) (*(gpio+((g)/10))&=~(7<<(((g)%10)*3)))
#define GPIO_OUT(g) (*(gpio+((g)/10))|=(1<<(((g)%10)*3)))

#define GPIO_SET(g) (*(gpio+7) = (1<<g))
#define GPIO_CLR(g) (*(gpio+10) = (1<<g))
#define GPIO_GET(g) (*(gpio+13)&(1<<g))
*/

#define GPIO_MAJOR 200
#define GPIO_MINOR 0
#define GPIO_DEVICE "gpioled"
#define GPIO_LED 17
#define GPIO_LED2 18
#define GPIO_SW 27
#define GPIO_SW2 22
#define BUF_SIZE 100

static char msg[BUF_SIZE] = {0};

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SEA");
MODULE_DESCRIPTION("Raspberry Pi First Device Driver");

//헤더에 존재하는 구조체
struct cdev gpio_cdev;
static int switch_irq;
static int switch_irq2;
static struct timer_list timer;         //타이머를 위한 구조체
static struct task_struct *task;        //task를 위한 구조체

pid_t pid;
char pid_valid;

static int gpio_open(struct inode *inod, struct file *fil);
static int gpio_close(struct inode *inod, struct file *fil);
static ssize_t gpio_write(struct file *inode, const char *buff, size_t len, loff_t *off);
static ssize_t gpio_read(struct file *inode, char *buff, size_t len, loff_t *off);

typedef struct Data_{
    char data;
}Data;

static struct file_operations gpio_fops = {
    .owner = THIS_MODULE,
    .read = gpio_read,
    .write = gpio_write,
    .open = gpio_open,
    .release = gpio_close,
};

volatile unsigned int *gpio;

static void timer_func(unsigned long data)
{
    printk(KERN_INFO "timer_func:%ld\n",data);

    gpio_set_value(GPIO_LED, data);

    if(data)
        timer.data = 0;
    else
        timer.data = 1;
    
    //timer.expires = jiffies + (1*HZ);
    timer.expires = jiffies + msecs_to_jiffies(500);
    add_timer(&timer);
}

static irqreturn_t isr_func(int irq, void *data)
{
    static int count;

    //IRQ발생

    if(irq == switch_irq && !gpio_get_value(GPIO_LED))

    {
        gpio_set_value(GPIO_LED,1);

        static struct siginfo sinfo;

        memset(&sinfo, 0, sizeof(struct siginfo));
        sinfo.si_signo = SIGIO;
        sinfo.si_code = SI_USER;

        task = pid_task(find_vpid(pid), PIDTYPE_PID);

        if(task != NULL)
            send_sig_info(SIGIO, &sinfo, task);

        else
            printk("Error : I don't know user pid\n");
    }
   
    else if (irq == switch_irq && gpio_get_value(GPIO_LED))
        gpio_set_value(GPIO_LED, 0);

    
    if(irq == switch_irq2 && !gpio_get_value(GPIO_LED2))

    {
        gpio_set_value(GPIO_LED2, 1);

        static struct siginfo sinfo;

        memset(&sinfo, 0, sizeof(struct siginfo));
        sinfo.si_signo = SIGIO;
        sinfo.si_code = SI_USER;

        task = pid_task(find_vpid(pid), PIDTYPE_PID);

        if(task != NULL)
            send_sig_info(SIGIO, &sinfo, task);

        else
            printk("Error : I don't know user pid\n");
    }

    else if(irq == switch_irq2 && gpio_get_value(GPIO_LED2))

        gpio_set_value(GPIO_LED2, 0);

    printk(KERN_INFO "GPIO Device write : %s\n", msg);



    /*if(irq == switch_irq && !gpio_get_value(GPIO_LED) && !gpio_get_value(GPIO_LED2))
    {
        gpio_set_value(GPIO_LED, 1);
           
        static struct siginfo sinfo;
        memset(&sinfo, 0, sizeof(struct siginfo));
        sinfo.si_signo = SIGIO;
        sinfo.si_code = SI_USER;

        task = pid_task(find_vpid(pid),PIDTYPE_PID); //구조체의 주소값을 알려줌

        if(task != NULL)
        {
            send_sig_info(SIGIO, &sinfo,task);
        }
        else
        {
            printk("Error : I don't know uwer pid\n");
        }

    }
        
    else if(irq == switch_irq && gpio_get_value(GPIO_LED) && !gpio_get_value(GPIO_LED2))
        gpio_set_value(GPIO_LED2, 1);
     

    else if(irq == switch_irq && gpio_get_value(GPIO_LED) && gpio_get_value(GPIO_LED2))
    { 
        gpio_set_value(GPIO_LED, 0);
        gpio_set_value(GPIO_LED2, 0);
    }*/
   
    /*
    //IRQ발생 && LED가 OFF일때
    if(irq == switch_irq && !gpio_get_value(GPIO_LED))
        gpio_set_value(GPIO_LED, 1);

    //IRQ발생 && LED가 ON일때
    else
        gpio_set_value(GPIO_LED, 0);
    */
    
    printk(KERN_INFO " Called isr_func() : %d\n", count);
    count++;
    
    return IRQ_HANDLED;
}

/*
int GPIO_SET(int g)
{
    if(g >31)
        *(gpio + 8) = (1 << (g - 31));
    else
        *(gpio + 7) = (1 << (g));

    return g;
}
*/
static int gpio_open(struct inode *inod, struct file *fil)
{
    try_module_get(THIS_MODULE);
    printk(KERN_INFO "GPIO Device opened()\n");
    return 0;
}

static int gpio_close(struct inode *inod, struct file *fil)
{
    // close()가 호출될때마다 모듈의 사용 카운터를 감소시킨다.
    module_put(THIS_MODULE);
    printk(KERN_INFO "GPIO Device Closed()\n");
    return 0;
}

static ssize_t gpio_write(struct file *inode, const char *buff, size_t len, loff_t *off)
{
    short count;
    char *cmd, *str;
    char *sep = ":";
    char *endptr, *pidstr;
    //pid_t pid;
    memset(msg, 0, BUF_SIZE);

    // 사용자영역(buff의 번지)에서 msg 배열로 데이터를 복사한다.
    count = copy_from_user(msg, buff, len);
    str = kstrdup(msg, GFP_KERNEL);    
    cmd = strsep(&str,sep);
    pidstr = strsep(&str, sep);
    
    cmd[1]='\0';
    
    printk("Command : %s, Pid : %s\n", cmd, pidstr);



    

    /*
    if((!strcmp(msg,"0")))
    {
        del_timer_sync(&timer);
    }

    else
    {
        init_timer(&timer);
        timer.function = timer_func;    //expire시 호출하는 함수
        timer.data = 1L;                 //timer_func으로 전달하는 인자값
        //timer.expires = jiffies + (1*HZ);                 //1초
        timer.expires = jiffies + msecs_to_jiffies(500);    //0.5초
        add_timer(&timer);
    }
    */

    //gpio_set_value(GPIO_LED, (!strcmp(msg,"0"))? 0 : 1);
    //gpio_set_value(GPIO_LED2, (!strcmp(msg,"2"))? 1 : 0);
    // = (!strcmp(msg,"0"))? GPIO_CLR(GPIO_LED):GPIO_SET(GPIO_LED);
    
    printk(KERN_INFO "GPIO Device write : %s\n", msg);
    
    //시그널 발생시 보낼 PID값을 등록
    pid = simple_strtol(pidstr, &endptr, 10);
    printk("pid = %d\n",pid);

    if(endptr != NULL)
    {
        task = pid_task(find_vpid(pid), PIDTYPE_PID);
    
        if(task == NULL)
        {
            printk("Error : I don't know user pid\n");
            return 0;
        }
    }


    return count;
}

static ssize_t gpio_read(struct file *inode, char *buff, size_t len, loff_t *off)
{
    short count;
//    int value;
    memset(msg,0,5);

    //GPIO_LED를 읽어옴
//    value = gpio_get_value(GPIO_LED);
//    if(value)
   
    //GPIO입력
   
    if(gpio_get_value(GPIO_SW))
        msg[0]='1';
    //else
      //  msg[0]='0';

    if(gpio_get_value(GPIO_SW2))
        msg[0]='2';
    //else
      //  msg[1]='0';

    strcat(msg, " from kernel");

    // 커널의 msg문자열을 사용자영역(buff의번지)으로 복사한다.
    // 사용자 메모리 블록 데이터를 커널 메모리 블록 데이터에 써넣는다
    count = copy_to_user(buff, msg, strlen(msg)+1);

    printk(KERN_INFO "GPIO Device read : %s\n", msg);

    /*
     * if(buff[0] == '1')
    {
        GPIO_SET(GPIO_LED1);
        GPIO_SET(GPIO_LED2);
    }
    else
    {
        GPIO_CLR(GPIO_LED1);
        GPIO_CLR(GPIO_LED2);
    }
    */

    return count;
}

static int initModule(void)
{
    dev_t devno;
    unsigned int count;
    int req;

    int err;
    //함수 호출 유무를 확인하기 위해
	printk(KERN_INFO "Init gpio_module\n");

    //1. 문자 디바이스를 등록
    devno = MKDEV(GPIO_MAJOR, GPIO_MINOR);
    printk(KERN_INFO "devno=0x%x\n", devno);
    register_chrdev_region(devno,1, GPIO_DEVICE);

    //2. 문자 디바이스의 번호와 이름을 등록
    cdev_init(&gpio_cdev, &gpio_fops);
    count = 1;

    //3. 문자 디바이스 추가
    err = cdev_add(&gpio_cdev, devno, count);
    if(err < 0)
    {
        printk(KERN_INFO "Error : cdev_add()\n");
        return -1;
    }

    printk(KERN_INFO "'mknod /dev/%s c %d 0'\n", GPIO_DEVICE, GPIO_MAJOR);
    printk(KERN_INFO "'chmod 666 /dev/%s'\n", GPIO_DEVICE);

    //gpio.h에 정의된 gpio_request함수의 사용
    //gpio_request = 특정 GPIO핀이 현재 다른 설정으로 사용되어 있는 지에 대한 여부를 판단하여 그 결과를 return함
    err = gpio_request(GPIO_LED, "LED");
    err = gpio_request(GPIO_LED2, "LED");

 
    // GPIO_SW를 IRQ로 설정
    err = gpio_request(GPIO_SW, "SW");
    err = gpio_request(GPIO_SW2, "SW");
  
    switch_irq = gpio_to_irq(GPIO_SW);
    switch_irq2 = gpio_to_irq(GPIO_SW2);

    err = request_irq(switch_irq, isr_func, IRQF_TRIGGER_RISING,"switch",NULL);
    err = request_irq(switch_irq2, isr_func, IRQF_TRIGGER_RISING,"switch",NULL);

    //4. 물리메모리 번지를 인자로 전달하면 가상메모리 번지를 리턴한다.
    //map = ioremap(GPIO_BASE, GPIO_SIZE);
   
    /*
    if(err==-EBUSY)
    {
        printk(KERN_INFO "Error: mapping GPIO memory\n");
        //iounmap(map);
        return -EBUSY;
    }
    */

    
    //gpio_direction_output = gpio포트를 output방향으로 초기화 하면서 초기값을 value로 설정
    gpio_direction_output(GPIO_LED, 0);
    gpio_direction_output(GPIO_LED2, 0);

    gpio_direction_input(GPIO_SW);
    gpio_direction_input(GPIO_SW2);
    // = GPIO_OUT(GPIO_LED1);
    
    //GPIO_OUT(GPIO_LED2);
    //GPIO_IN(GPIO_SW);

	return 0;
}

static void __exit cleanupModule(void)
{
    dev_t devno = MKDEV(GPIO_MAJOR, GPIO_MINOR);
    del_timer_sync(&timer);
    
    //1. 문자 디바이스의 등록(장치번호, 장치명)을 해제한다.
    unregister_chrdev_region(devno, 1);

    //2. 문자 디바이스의 구조체를 제거한다.
    cdev_del(&gpio_cdev);

    //gpio_direction_output(GPIO_LED, 0);
    //gpio_direction_output(GPIO_LED2,0);
    //gpio_direction_input(GPIO_SW,0);
    
    free_irq(switch_irq,NULL);
    free_irq(switch_irq2,NULL);
    gpio_set_value(GPIO_LED, 0);
    gpio_set_value(GPIO_LED2, 0);
    gpio_free(GPIO_LED);
    gpio_free(GPIO_LED2);
    gpio_free(GPIO_SW);
    gpio_free(GPIO_SW2);

	printk(KERN_INFO "Exit gpio_module : Good Bye\n");
}

//my initialize function
module_init(initModule);
//my exit function
module_exit(cleanupModule);
