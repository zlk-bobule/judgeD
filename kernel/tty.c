
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               tty.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

#define TTY_FIRST	(tty_table)
#define TTY_END		(tty_table + NR_CONSOLES)

#define TRUE 1
#define FALSE 0

PRIVATE void init_tty(TTY* p_tty);
PRIVATE void tty_do_read(TTY* p_tty);
PRIVATE void tty_do_write(TTY* p_tty);
PRIVATE void put_key(TTY* p_tty, u32 key);
PRIVATE int searchCharNum;
PUBLIC int isEscState;
PRIVATE int isShield;
extern char console_color;

/*======================================================================*
                           task_tty
 *======================================================================*/
PUBLIC void task_tty()
{
	
	searchCharNum = 0;
	isEscState = FALSE;
	isShield = FALSE;
	
	TTY*	p_tty;

	init_keyboard();

	for (p_tty=TTY_FIRST;p_tty<TTY_END;p_tty++) {
		init_tty(p_tty);
	}
	select_console(0);
	//clean_console(p_tty->p_console);
	while (1) {
		//循环进行控制台读写操作
		for (p_tty=TTY_FIRST;p_tty<TTY_END;p_tty++) {
			tty_do_read(p_tty);
			tty_do_write(p_tty);
		}
	}
}

/*======================================================================*
			   init_tty
 *======================================================================*/
PRIVATE void init_tty(TTY* p_tty)
{
	//清空缓存区
	p_tty->inbuf_count = 0;
	p_tty->p_inbuf_head = p_tty->p_inbuf_tail = p_tty->in_buf;

	init_screen(p_tty);
}

/*======================================================================*
				in_process
 *======================================================================*/
PUBLIC void in_process(TTY* p_tty, u32 key)
{
        char output[2] = {'\0', '\0'};
	
		/*if(isEscState) {
			//如果按下esc键进入查找状态，则对输入的搜索条件进行计数
			if(!isShield && ((key & MASK_RAW) != ESC) && ((key & MASK_RAW) != ESC) && ((key & MASK_RAW) != ENTER)) {
				searchCharNum++;
			}
		}*/
		

        if (!(key & FLAG_EXT)) {
			if(!isShield) {
				put_key(p_tty, key);
			}
			if(isEscState) {
				searchCharNum++;
			}
        }
        else {
        	int raw_code = key & MASK_RAW;
            switch(raw_code) {
                case ENTER:
					if(isEscState) {
						isShield = TRUE;
						search_console(p_tty->p_console, searchCharNum);
					} else {
						put_key(p_tty, '\n');
					}
					break;
                case BACKSPACE:
					if(isEscState) {
						if(searchCharNum > 0){	
							searchCharNum--;
							put_key(p_tty, '\b');
						}
					} else {
						put_key(p_tty, '\b');
					}
					
					break;
                case UP:
                    if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
							scroll_screen(p_tty->p_console, SCR_DN);
                        }
					break;
				case DOWN:
					if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
						scroll_screen(p_tty->p_console, SCR_UP);
					}
					break;
				case F1:
				case F2:
				case F3:
				case F4:
				case F5:
				case F6:
				case F7:
				case F8:
				case F9:
				case F10:
				case F11:
				case F12:
					/* Alt + F1~F12 */
					if ((key & FLAG_ALT_L) || (key & FLAG_ALT_R)) {
						select_console(raw_code - F1);
					}
					break;
				case TAB:
					//tab键用'\t'表示
					if(!isShield) {
						put_key(p_tty, '\t');
					}
					break;
					
				case ESC:
					//isEscState = (isEscState == TRUE) ? FALSE : TRUE;
					if(isEscState) {
						//退出搜索状态，清空搜索字符，恢复原来的颜色
						changeDefaultColor(p_tty->p_console);
						isShield = FALSE;
						isEscState = FALSE;
						for(int i = 0; i < searchCharNum; i++) {
							put_key(p_tty, '\b');
						}
						searchCharNum = 0;//清空搜索字符
					} else {
						searchCharNum = 0;//清空搜索字符
						isEscState = TRUE;
					}
					break;
                default:
                    break;
                }
        }
}

/*======================================================================*
			      put_key
*======================================================================*/
PRIVATE void put_key(TTY* p_tty, u32 key)
{
	if (p_tty->inbuf_count < TTY_IN_BYTES) {
		*(p_tty->p_inbuf_head) = key;
		p_tty->p_inbuf_head++;
		if (p_tty->p_inbuf_head == p_tty->in_buf + TTY_IN_BYTES) {
			p_tty->p_inbuf_head = p_tty->in_buf;
		}
		p_tty->inbuf_count++;
	}
}


/*======================================================================*
			      tty_do_read
 *======================================================================*/
PRIVATE void tty_do_read(TTY* p_tty)
{
	if (is_current_console(p_tty->p_console)) {
		keyboard_read(p_tty);
	}
}


/*======================================================================*
			      tty_do_write
 *======================================================================*/
PRIVATE void tty_do_write(TTY* p_tty)
{
	if (p_tty->inbuf_count) {
		char ch = *(p_tty->p_inbuf_tail);
		p_tty->p_inbuf_tail++;
		if (p_tty->p_inbuf_tail == p_tty->in_buf + TTY_IN_BYTES) {
			p_tty->p_inbuf_tail = p_tty->in_buf;
		}
		p_tty->inbuf_count--;

		out_char(p_tty->p_console, ch);
	}
}


