#include <stdio.h>

#include <sys/types.h>

#include <sys/stat.h>

#include <fcntl.h>

#include <sys/mman.h>

#include <unistd.h>

#include <linux/input.h>

#include <stdlib.h>



unsigned char *lcd_ptr;

int ts_fd;

int get_xy(int *x, int *y)
{
    struct input_event ts;
    int x_read = 0, y_read = 1;

    *x = 0, *y = 0;
    
    //2, 打印 触摸屏相关信息
    while (1) {
        read(ts_fd, &ts, sizeof(ts));
        
        //绝对坐标事件 （x, y） (x, x) (y, y) (y, x)
        if (ts.type == EV_ABS) {
            if (ts.code == ABS_X && x_read == 0) {
                *x = ts.value;
                x_read = 1;
                y_read = 0;
            }
            if (ts.code == ABS_Y && y_read == 0) {
                *y = ts.value;
                x_read = 0;
                y_read = 1;
            }
        }
        
        if (*x != 0 && *y != 0) {
            break;
        }

        /* if (ts.type == EV_KEY) {
            if (ts.code == BTN_TOUCH) {
                if (ts.value == KEY_RESERVED) {
                    break;
                }
            }
        }
        */
    }
    
    
}



int lcd_draw_bmp(const char *pathname, int x, int y, int w, int h)

{

    int i, j;

    

    //a 打开图片文件

    

    int bmp_fd = open(pathname, O_RDWR);

    

    //错误处理

    

    if (bmp_fd == -1) {

        printf("open bmp file failed!\n");

        return -1;

    }

    

    //2，将图片数据加载到lcd屏幕

    char header[54];

    

    char rgb_buf[w*h*3];
    //char argb_buf[800*480*4];

    //a 将图片颜色数据读取出来

    

    read(bmp_fd, header, 54);


    /* 1, 文件头 54字节 文件信息存储 宽，高，位深度,... */


    int pad = (4-(w*3)%4)%4;


    /* 2, 位深度：24位：红，绿，蓝 */


    for (i = 0; i < h; i++) {

        //读取图片颜色数据

        read(bmp_fd, &rgb_buf[w*i*3], w*3);

        //跳过无效字节

        lseek(bmp_fd, pad, SEEK_CUR);

    }

    //b 加载数据到lcd屏幕

    

    

    // int r = 0xef, g = 0xab, b = 0xcd;

    // int color = 0xefabcd;

    

    //int color = b;

    

    // 遇1结果则为1

    //     b : 00000000 00000000 00000000 11001101

    //     g : 00000000 00000000 10101011 00000000

    // color : 00000000 00000000 10101011 11001101

    

    // 1000  = 800*1+200

    // 1800  = 800*2+200

    

    //24 --- 32

    for (j = 0; j < h; j++) {

        for (i = 0; i < w; i++) {

            /* 24 --》 32 */

            /* 800*j+i 提取像素点 */

            /* 3, bmp 存储方式：从左往右，从下往上 */

            *(lcd_ptr+(800*(h-1-j+y)+i+x)*4+0) = rgb_buf[(j*w+i)*3+0];

            *(lcd_ptr+(800*(h-1-j+y)+i+x)*4+1) = rgb_buf[(j*w+i)*3+1];

            *(lcd_ptr+(800*(h-1-j+y)+i+x)*4+2) = rgb_buf[(j*w+i)*3+2];

            *(lcd_ptr+(800*(h-1-j+y)+i+x)*4+3) = 0;

            /*
            * argb_buf[(800*(h-1-j+y)+i+x)*4+0] = rgb_buf[(j*w+i)*3+0];
            * argb_buf[(800*(h-1-j+y)+i+x)*4+1] = rgb_buf[(j*w+i)*3+1];
            * argb_buf[(800*(h-1-j+y)+i+x)*4+2] = rgb_buf[(j*w+i)*3+2];
            * argb_buf[(800*(h-1-j+y)+i+x)*4+3] = 0;
            */

        }

    }

    

    

    //3，关闭文件

    //a 关闭图片文件

    close(bmp_fd);

    

    return 0;

}



int main(void)

{

    //1,打开设备文件

    

    int lcd_fd = open("/dev/ubuntu_lcd", O_RDWR);

    

    //错误处理

    if (lcd_fd == -1) {

        printf("open lcd device failed!\n");

        return -1;

    }

    ts_fd = open("/dev/ubuntu_event", O_RDWR);

    

    //错误处理

    if (ts_fd == -1) {

        printf("open ts device failed!\n");

        return -1;

    }

    

    //2,为lcd设备建立内存映射关系:镜子

    lcd_ptr = mmap(NULL, 800*480*4, PROT_READ | PROT_WRITE, MAP_SHARED, lcd_fd, 0);

    

    if (lcd_ptr == MAP_FAILED) {

        printf("mmap failed!\n");

        return -1;

    }

    //显示背景
    lcd_draw_bmp("backgound.bmp", 0, 0, 800, 480);
    lcd_draw_bmp("next.bmp", 20, 390, 80, 80);
    lcd_draw_bmp("prev.bmp", 680, 390, 80, 80);

    char music_buf[4096];

    const char *pic_name[] = {"1.bmp", "2.bmp", "3.bmp", "4.bmp"};
    const char *music_name[] = {"1.mp3", "2.mp3", "3.mp3", "4.mp3"}; 

    int count = 0;

    int x, y;

    while (1) {
        get_xy(&x, &y);
        printf("(%d, %d)\n", x, y);

        /* 上一张图片切换的同时并播放上一首歌 */
        if (x >= 0 && x < 400 && y >= 0 && y < 480) {
            // 让当前歌停止然后播放下一首
            system("killall -SIGKILL mplayer");
            
            count--;
            if (count == -1) {count = 3;}

            lcd_draw_bmp(pic_name[count], 20, 10, 760, 370);

            sprintf(music_buf, "mplayer %s &", music_name[count]);
            system(music_buf);
        }else if (x >= 400 && x < 800 && y >= 0 && y < 480) {
            system("killall -SIGKILL mplayer");
count++;
if(count==4){count=-1;		}
lcd_draw_bmp(pic_name[count],20,10,760,370);
sprintf(music_buf,"mplayer %s &",music_name[count]);
system(music_buf);
    }
else if(x>=400&&x<800&&y>=0&&y<480)
{system("killall -SIGKILL mplayer");
}
}

    munmap(lcd_ptr, 800*480*4);

    //b 关闭设备文件

    close(lcd_fd);

    

    return 0;

}

//上面有些源码的格式可能有所偏差，需要自己去换行以下
