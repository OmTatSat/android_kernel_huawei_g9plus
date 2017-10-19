#include "ilitek_ts.h"
#ifdef GESTURE
static int system_resume_ilitek = 1;
#endif

#ifdef REPORT_THREAD
static DECLARE_WAIT_QUEUE_HEAD(waiter);
static int tpd_flag = 0;
#endif

#ifdef ILI_UPDATE_FW
static int update_wait_flag = 0;
extern int ilitek_upgrade_firmware(void);
#endif

#ifdef TOOL
struct dev_data {
	dev_t devno;
	struct cdev cdev;
	struct class *class;
};

extern struct dev_data dev_ilitek;
extern struct proc_dir_entry * proc_ilitek;
extern int create_tool_node(void);
extern int remove_tool_node(void);
#endif

#ifdef SENSOR_TEST
#define SYS_ATTR_FILE
#ifdef SYS_ATTR_FILE
extern struct kobject *android_touch_kobj;
extern void ilitek_touch_node_deinit(void);
extern void ilitek_touch_node_init(void);
#endif
#endif

#ifdef HALL_CHECK
extern void ilitek_hall_check_hw_init(void);
extern void ilitek_hall_check_hw_deinit(void);
#endif
 int touch_key_hold_press = 0;
 int touch_key_code[] = {KEY_MENU,KEY_HOMEPAGE,KEY_BACK,KEY_VOLUMEDOWN,KEY_VOLUMEUP};
 int touch_key_press[] = {0, 0, 0, 0, 0};
 unsigned long touch_time=0;
 int driver_information[] = {DERVER_VERSION_MAJOR,DERVER_VERSION_MINOR,CUSTOMER_ID,MODULE_ID,PLATFORM_ID,PLATFORM_MODULE,ENGINEER_ID};
 char Report_Flag;
 volatile char int_Flag;
 volatile char update_Flag;
 int update_timeout;

 char EXCHANG_XY = 0;
 char REVERT_X = 0;
 char REVERT_Y = 0;
 char DBG_FLAG,DBG_COR;
struct i2c_data i2c;
#define ILITEK_JDI_MODULE 8
#define X_CHANNEL_NUM 15
#define Y_CHANNEL_NUM 27
#define MAX_X 1079
#define MAX_Y 1919
#define MAX_TOUCH_POINT 10
#define JUDGE_ZERO 0
static struct regulator *syn_power_vbus = NULL;
static struct regulator *syn_power_vdd = NULL;
static int set_command(struct i2c_client *client, uint8_t *cmd, int write_len, int delay, uint8_t *data, int read_len);
static void ilitek_set_input_param(struct input_dev*, int max_tp, int max_x, int max_y);
static int ilitek_i2c_process_and_report(void);
static int ilitek_i2c_probe(struct i2c_client*, const struct i2c_device_id*);
static int ilitek_i2c_remove(struct i2c_client*);
#if defined(CONFIG_FB)
static int fb_notifier_callback(struct notifier_block *self,
unsigned long event, void *data);
#elif defined(CONFIG_HAS_EARLYSUSPEND)
static void ilitek_i2c_early_suspend(struct early_suspend *h);
static void ilitek_i2c_late_resume(struct early_suspend *h);
#endif
static int ilitek_i2c_polling_thread(void*);
static irqreturn_t ilitek_i2c_isr(int, void*);
static void ilitek_i2c_irq_work_queue_func(struct work_struct*);
static int Request_IRQ(void);
static int ilitek_request_init_reset(void);
static int ilitek_init(void);
static void ilitek_exit(void);
static const struct i2c_device_id ilitek_i2c_id[] ={
	{ILITEK_I2C_DRIVER_NAME, 0}, {}
};
MODULE_DEVICE_TABLE(i2c, ilitek_i2c_id);

static struct of_device_id ilitek_match_table[] = {
	{ .compatible = "ilitek,2120",},
	{ .compatible = "ilitek,2139",},
	{ .compatible = "ilitek,2839",},
	{ .compatible = "ilitek,2113",},
	{ .compatible = "ilitek,2115",},
	{ .compatible = "ilitek,2116",},
	{ .compatible = "ilitek,2656",},
	{ .compatible = "ilitek,2645",},
	{ .compatible = "ilitek,2302",},
	{ .compatible = "ilitek,2303",},
	{ .compatible = "ilitek,2102",},
	{ .compatible = "ilitek,2111",},
};

static struct i2c_driver ili2120_ts_driver = {
	.probe = ilitek_i2c_probe,
	.remove = ilitek_i2c_remove,
#ifdef CONFIG_FB
	.suspend = ilitek_i2c_suspend,
	.resume = ilitek_i2c_resume,
#endif
	.driver = {
		.name = "ilitek_i2c",
		.owner = THIS_MODULE,
		.of_match_table = ilitek_match_table,
	},
	.id_table = ilitek_i2c_id,
};

void ilitek_reset(int reset_gpio)
{
	tp_log_info("Enter ilitek_reset\n");
	gpio_direction_output(reset_gpio,1);
	//msleep(50);
	mdelay(50);
	gpio_direction_output(reset_gpio,0);
	//msleep(50);
	mdelay(50);
	gpio_direction_output(reset_gpio,1);
	//msleep(100);
	mdelay(100);
	return;
}

static int ilitek_request_init_reset(void)
{
	s32 ret=0;
	ilitek_reset(i2c.reset_gpio);
	return ret;
}

static int Request_IRQ(void)
{
	int ret = 0;
	ret = request_irq(i2c.client->irq, ilitek_i2c_isr, IRQF_TRIGGER_FALLING/*| IRQF_SHARED*/ , "ilitek_i2c_irq", &i2c);
	return ret;
}

static void ilitek_set_input_param(struct input_dev *input,	int max_tp, int max_x, int max_y)
{
	int key;
	__set_bit(INPUT_PROP_DIRECT, input->propbit);
	input->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
	#ifndef ROTATE_FLAG
		input_set_abs_params(input, ABS_MT_POSITION_X, 0, max_x, 0, 0);
		input_set_abs_params(input, ABS_MT_POSITION_Y, 0, max_y, 0, 0);
	#else
		input_set_abs_params(input, ABS_MT_POSITION_X, 0, max_y, 0, 0);
		input_set_abs_params(input, ABS_MT_POSITION_Y, 0, max_x, 0, 0);
	#endif
		input_set_abs_params(input, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
		input_set_abs_params(input, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
#ifdef TOUCH_PROTOCOL_B
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,7,0)
		input_mt_init_slots(input, max_tp, INPUT_MT_DIRECT);
	#else
		input_mt_init_slots(input, max_tp);
	#endif
#else
		input_set_abs_params(input, ABS_MT_TRACKING_ID, 0, max_tp, 0, 0);
#endif
#ifdef REPORT_PRESSURE
	input_set_abs_params(input, ABS_MT_PRESSURE, 0, 255, 0, 0);
#endif
	for(key=0; key<sizeof(touch_key_code) / sizeof(touch_key_code[0]); key++){
			if(touch_key_code[key] <= 0){
					continue;
		}
			set_bit(touch_key_code[key] & KEY_MAX, input->keybit);
	}
	input->name = ILITEK_I2C_DRIVER_NAME;
	input->id.bustype = BUS_I2C;
	input->dev.parent = &(i2c.client)->dev;
}

int ilitek_i2c_transfer(struct i2c_client *client, struct i2c_msg *msgs, int cnt)
{
	int ret, count=ILITEK_I2C_RETRY_COUNT;

	while(count >= 0){
		count-= 1;
		ret = down_interruptible(&i2c.wr_sem);
		ret = i2c_transfer(client->adapter, msgs, cnt);
		up(&i2c.wr_sem);
		if(ret < 0){
			msleep(500);
			continue;
		}
		break;
	}
	return ret;
}

int ilitek_i2c_read(struct i2c_client *client, uint8_t cmd, uint8_t *data, int length)
{
	int ret;
	struct i2c_msg msgs_cmd[] = {
		{.addr = client->addr, .flags = 0, .len = 1, .buf = &cmd,},
	};

	struct i2c_msg msgs_ret[] = {
		{.addr = client->addr, .flags = I2C_M_RD, .len = length, .buf = data,}
	};

	ret = ilitek_i2c_transfer(client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d, i2c read error, ret %d\n", __func__,__LINE__,ret);
	}

	ret = ilitek_i2c_transfer(client, msgs_ret, 1);
	if(ret < 0){
		tp_log_err("%s:%d, i2c read error, ret %d\n", __func__,__LINE__,ret);
	}
	return ret;
}

static int ilitek_i2c_process_and_report(void)
{
#ifdef ROTATE_FLAG
	int org_x = 0, org_y = 0;
#endif
	int i, len = 0, ret, x = 0, y = 0/*, mult_tp_id*/, tp_status = 0;
#ifdef REPORT_PRESSURE
	int pressure = 0;
#endif
#ifdef VIRTUAL_KEY_PAD
	unsigned char key_id = 0;
#endif
#ifndef TOUCH_PROTOCOL_B
		int release_counter = 0;
#endif
	unsigned char buf[64]={0};
	if(i2c.report_status == 0){
		return 1;
	}

	buf[0] = ILITEK_TP_CMD_READ_DATA;
	ret = set_command(i2c.client, buf, 1, 0, buf, 53);
	len = buf[0];
	if (ret < 0) {
		tp_log_info("ilitek ILITEK_TP_CMD_READ_DATA error return & release\n");
		#ifdef TOUCH_PROTOCOL_B
		for(i = 0; i < i2c.max_tp; i++)
		{
			if(i2c.touchinfo[i].flag == 1)
			{
				tp_log_info("ilitek release point i = %d\n", i);
#ifdef REPORT_PRESSURE
				input_event(i2c.input_dev, EV_ABS, ABS_MT_PRESSURE, 0);
#endif
				input_mt_slot(i2c.input_dev, i);
				input_mt_report_slot_state(i2c.input_dev, MT_TOOL_FINGER, false);
			}
			i2c.touchinfo[i].flag = 0;
		}
		#else
		if(i2c.touch_flag == 1)
		{
			i2c.touch_flag = 0;
			//tp_log_debug("Release, ID=%02X, X=%04d, Y=%04d\n", buf[0] & 0x3F, x, y);
#ifdef REPORT_PRESSURE
			input_event(i2c.input_dev, EV_ABS, ABS_MT_PRESSURE, 0);
#endif
			input_report_key(i2c.input_dev, BTN_TOUCH,	0);
			input_mt_sync(i2c.input_dev);
		}
		#endif
		return ret;
	}
	else {
		ret = 0;
	}
	len = buf[0];
	//tp_log_info("ilitek len = 0x%x buf[0] = 0x%x, buf[1] = 0x%x, buf[2] = 0x%x\n", len, buf[0], buf[1], buf[2]);
	if (len > 20) {
		tp_log_info("ilitek len > 20  return & release\n");
		#ifdef TOUCH_PROTOCOL_B
		for(i = 0; i < i2c.max_tp; i++)
		{
			if(i2c.touchinfo[i].flag == 1)
			{
				//tp_log_info("ilitek release point i = %d\n", i);
#ifdef REPORT_PRESSURE
				input_event(i2c.input_dev, EV_ABS, ABS_MT_PRESSURE, 0);
#endif
				input_mt_slot(i2c.input_dev, i);
				input_mt_report_slot_state(i2c.input_dev, MT_TOOL_FINGER, false);
			}
			i2c.touchinfo[i].flag = 0;
		}
		#else
		if(i2c.touch_flag == 1)
		{
			i2c.touch_flag = 0;
			//tp_log_debug("Release, ID=%02X, X=%04d, Y=%04d\n", buf[0] & 0x3F, x, y);
#ifdef REPORT_PRESSURE
			input_event(i2c.input_dev, EV_ABS, ABS_MT_PRESSURE, 0);
#endif
			input_report_key(i2c.input_dev, BTN_TOUCH,	0);
			input_mt_sync(i2c.input_dev);
		}
		#endif
		return ret;
	}
	#ifdef GESTURE
	if (system_resume_ilitek == 0) {
		tp_log_info("gesture wake up 0x%x, 0x%x, 0x%x\n", buf[0], buf[1], buf[2]);
		if (buf[2] == 0x60) {
			tp_log_info("gesture wake up this is c\n");
			input_report_key(i2c.input_dev, KEY_C, 1);
			input_sync(i2c.input_dev);
			input_report_key(i2c.input_dev, KEY_C, 0);
			input_sync(i2c.input_dev);
		}
		if (buf[2] == 0x62) {
			tp_log_info("gesture wake up this is e\n");
			input_report_key(i2c.input_dev, KEY_E, 1);
			input_sync(i2c.input_dev);
			input_report_key(i2c.input_dev, KEY_E, 0);
			input_sync(i2c.input_dev);
		}
		if (buf[2] == 0x64) {
			tp_log_info("gesture wake up this is m\n");
			input_report_key(i2c.input_dev, KEY_M, 1);
			input_sync(i2c.input_dev);
			input_report_key(i2c.input_dev, KEY_M, 0);
			input_sync(i2c.input_dev);
		}
		if (buf[2] == 0x66) {
			tp_log_info("gesture wake up this is w\n");
			input_report_key(i2c.input_dev, KEY_W, 1);
			input_sync(i2c.input_dev);
			input_report_key(i2c.input_dev, KEY_W, 0);
			input_sync(i2c.input_dev);
		}
		if (buf[2] == 0x68) {
			tp_log_info("gesture wake up this is o\n");
			input_report_key(i2c.input_dev, KEY_O, 1);
			input_sync(i2c.input_dev);
			input_report_key(i2c.input_dev, KEY_O, 0);
			input_sync(i2c.input_dev);
		}
		input_report_key(i2c.input_dev, KEY_POWER, 1);
		input_sync(i2c.input_dev);
		input_report_key(i2c.input_dev, KEY_POWER, 0);
		input_sync(i2c.input_dev);
		return 0;
	}
	#endif
	for(i = 0; i < i2c.max_tp; i++){
		tp_status = buf[i*5+3] >> 7;	
		x = (((int)(buf[i*5+3] & 0x3F) << 8) + buf[i*5+4]);
		y = (((int)(buf[i*5+5] & 0x3F) << 8) + buf[i*5+6]);
		#ifdef REPORT_PRESSURE
		pressure = buf[i * 5 + 7];
		#endif
		if (tp_status) {
			//tp_log_info("ilitek TOUCH_POINT  i = %d  \n", i);
			//report to android system
			//tp_log_debug("Point, ID=%02X, X=%04d, Y=%04d,touch_key_hold_press=%d\n",buf[0]  & 0x3F, x,y,touch_key_hold_press);
#ifdef TOUCH_PROTOCOL_B
			input_mt_slot(i2c.input_dev, i);
			input_mt_report_slot_state(i2c.input_dev, MT_TOOL_FINGER, true);
			i2c.touchinfo[i].flag = 1;
#endif
			input_event(i2c.input_dev, EV_ABS, ABS_MT_POSITION_X, x);
			input_event(i2c.input_dev, EV_ABS, ABS_MT_POSITION_Y, y);
			input_event(i2c.input_dev, EV_ABS, ABS_MT_TOUCH_MAJOR, 1);
#ifdef REPORT_PRESSURE
			input_event(i2c.input_dev, EV_ABS, ABS_MT_PRESSURE, pressure);
#endif
#ifndef TOUCH_PROTOCOL_B
			input_event(i2c.input_dev, EV_ABS, ABS_MT_TRACKING_ID, i);
			input_report_key(i2c.input_dev, BTN_TOUCH, 1);
			input_mt_sync(i2c.input_dev);
			i2c.touch_flag = 1;
#endif
			ret = 0;
		}
		else {
			//tp_log_info("ilitek RELEASE_POINT  i = %d  \n", i);
#ifdef TOUCH_PROTOCOL_B
			if(i2c.touchinfo[i].flag == 1)
			{
				//tp_log_info("ilitek release point okokok i = %d\n", i);
#ifdef REPORT_PRESSURE
				input_event(i2c.input_dev, EV_ABS, ABS_MT_PRESSURE, 0);
#endif
				input_mt_slot(i2c.input_dev, i);
				input_mt_report_slot_state(i2c.input_dev, MT_TOOL_FINGER, false);
				i2c.touchinfo[i].flag = 0;
			}
#else
			release_counter++;
			#if 0
			if(release_counter == len)
			{
				input_report_key(i2c.input_dev, BTN_TOUCH, 0);
				input_mt_sync(i2c.input_dev);
				i2c.touch_flag = 0;
			}
			#endif
#ifdef REPORT_PRESSURE
				input_event(i2c.input_dev, EV_ABS, ABS_MT_PRESSURE, 0);
				input_mt_sync(i2c.input_dev);
#endif
#endif
		}
	}
	if(len == 0)
	{
#ifdef TOUCH_PROTOCOL_B
		for(i = 0; i < i2c.max_tp; i++)
		{
			if(i2c.touchinfo[i].flag == 1)
			{
				//tp_log_info("ilitek release point i = %d\n", i);
#ifdef REPORT_PRESSURE
				input_event(i2c.input_dev, EV_ABS, ABS_MT_PRESSURE, 0);
#endif
				input_mt_slot(i2c.input_dev, i);
				input_mt_report_slot_state(i2c.input_dev, MT_TOOL_FINGER, false);
			}
			i2c.touchinfo[i].flag = 0;
		}
#else
		if(i2c.touch_flag == 1)
		{
			i2c.touch_flag = 0;
			//tp_log_debug("Release, ID=%02X, X=%04d, Y=%04d\n", buf[0] & 0x3F, x, y);
#ifdef REPORT_PRESSURE
			input_event(i2c.input_dev, EV_ABS, ABS_MT_PRESSURE, 0);
#endif
			input_report_key(i2c.input_dev, BTN_TOUCH,  0);
			input_mt_sync(i2c.input_dev);
		}
#endif
	}
	//tp_log_debug("%s,ret=%d\n",__func__,ret);

#ifdef TOUCH_PROTOCOL_B
	input_mt_report_pointer_emulation(i2c.input_dev, true);
#endif
	input_sync(i2c.input_dev);
	return ret;
}

#ifndef CLOCK_INTERRUPT
static void ilitek_i2c_timer(unsigned long handle)
{
	struct i2c_data *priv = (void *)handle;
	tp_log_debug("Enter\n");
	schedule_work(&priv->irq_work);
}
#endif

static void ilitek_i2c_irq_work_queue_func(struct work_struct *work)
{
	int ret;
#ifndef CLOCK_INTERRUPT
	struct i2c_data *priv = container_of(work, struct i2c_data, irq_work);
#endif
	tp_log_debug("Enter\n");

	ret = ilitek_i2c_process_and_report();
#ifdef CLOCK_INTERRUPT
	ilitek_i2c_irq_enable();
#else
	if (ret == 0){
		if (!i2c.stop_polling)
			mod_timer(&priv->timer, jiffies + msecs_to_jiffies(0));
	}
	else if (ret == 1){
		if (!i2c.stop_polling){
			ilitek_i2c_irq_enable();
		}
		tp_log_debug("stop_polling\n");
	}
	else if(ret < 0){
		msleep(100);
		tp_log_err("%s, process error\n", __func__);
		ilitek_i2c_irq_enable();
	}
#endif
}

static irqreturn_t ilitek_i2c_isr(int irq, void *dev_id)
{
	//tp_log_err("%s Enter\n",__func__);
	if(i2c.firmware_updating) {
		tp_log_info("%s: tp fw is updating,return\n", __func__);
		return IRQ_HANDLED;
	}
	if(i2c.irq_status ==1){
		disable_irq_nosync(i2c.client->irq);
		i2c.irq_status = 0;
	}
	if(update_Flag == 1){
		int_Flag = 1;
	}
	else{
		#ifndef REPORT_THREAD
		queue_work(i2c.irq_work_queue, &i2c.irq_work);
		#else
		tpd_flag = 1;
		wake_up_interruptible(&waiter);
		#endif
	}

	return IRQ_HANDLED;
}
#ifdef REPORT_THREAD

static int ilitek_i2c_touchevent_thread(void *arg)
{
	int ret=0;
	struct sched_param param = { .sched_priority = 4};
	sched_setscheduler(current, SCHED_RR, &param);	
	tp_log_info("%s, enter\n", __func__);

	while(1){
		//tp_log_info("%s, enter tpd_flag = %d\n", __func__, tpd_flag);
		if(kthread_should_stop()){
			tp_log_info("%s, stop\n", __func__);
			break;
		}
		set_current_state(TASK_INTERRUPTIBLE);
		wait_event_interruptible(waiter, tpd_flag != 0);
		tpd_flag = 0;
		set_current_state(TASK_RUNNING);
		if(ilitek_i2c_process_and_report() < 0){
			msleep(3000);
			tp_log_err("%s, process error\n", __func__);
		}
		#ifdef CLOCK_INTERRUPT
		ilitek_i2c_irq_enable();
		#endif
	}
	tp_log_info("%s, exit\n", __func__);
	return ret;
}
#endif
#ifdef ILI_UPDATE_FW

static int ilitek_i2c_update_thread(void *arg)
{

	int ret=0;
	tp_log_info("%s, enter\n", __func__);

	if(kthread_should_stop()){
		tp_log_info("%s, stop\n", __func__);
		return -1;
	}
	
	disable_irq(i2c.client->irq);
	tp_log_info("ilitek disable irq\n");
	#if 1
	update_wait_flag = 1;
	i2c.firmware_updating = true;
	ret = ilitek_upgrade_firmware();
	if(ret==2) {
		tp_log_info("ilitek update end\n");
	}
	else if(ret==3) {
		tp_log_info("ilitek i2c communication error\n");
	}
	//if(i2c.reset_request_success){
	//	ilitek_reset(i2c.reset_gpio);
	//}
	mdelay(200);
	i2c.firmware_updating = false;
	#endif
	ret=ilitek_i2c_read_tp_info();
	if(ret < 0)
	{
		return ret;
	}
	
	enable_irq(i2c.client->irq);
	tp_log_info("ilitek enable irq\n");
	update_wait_flag = 0;
	ilitek_set_input_param(i2c.input_dev, i2c.max_tp, i2c.max_x, i2c.max_y);
	ret = input_register_device(i2c.input_dev);
	if(ret){
		tp_log_info("%s, register input device, error\n", __func__);
		return ret;
	}
	tp_log_info("%s, register input device, success\n", __func__);
	return ret;
}
#endif

static int ilitek_i2c_polling_thread(void *arg)
{

	int ret=0;
	tp_log_info("%s, enter\n", __func__);

	while(1){
		if(kthread_should_stop()){
			tp_log_info("%s, stop\n", __func__);
			break;
		}
		msleep(10);
		if(i2c.stop_polling){
			msleep(1000);
			continue;
		}

		if(ilitek_i2c_process_and_report() < 0){
			msleep(3000);
			tp_log_err("%s, process error\n", __func__);
		}
	}
	tp_log_info("%s, exit\n", __func__);
	return ret;
}

#if defined(CONFIG_FB)
static void ilitek_ts_suspend(struct i2c_data *ts)
{
	ilitek_i2c_suspend(i2c.client, PMSG_SUSPEND);
	tp_log_info("%s\n", __func__);
}

static void ilitek_ts_resume(struct i2c_data *ts)
{
	ilitek_i2c_resume(i2c.client);
	tp_log_info("%s\n", __func__);
}

static int fb_notifier_callback(struct notifier_block *self,
		unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int *blank;
	struct i2c_data *ts =
		container_of(self, struct i2c_data, fb_notif);

	if (evdata && evdata->data && event == FB_EVENT_BLANK && ts && ts->client) {
		blank = evdata->data;
		if (*blank == FB_BLANK_UNBLANK) {
			tp_log_info("fb_notifier_callback ilitek_ts_resume\n");
			ilitek_ts_resume(ts);
		}
		else if (*blank == FB_BLANK_POWERDOWN) {
			tp_log_info("fb_notifier_callback ilitek_ts_suspend\n");
			ilitek_ts_suspend(ts);
		}
	}
	return 0;
}

#elif defined(CONFIG_HAS_EARLYSUSPEND)
static void ilitek_i2c_early_suspend(struct early_suspend *h)
{
	ilitek_i2c_suspend(i2c.client, PMSG_SUSPEND);
	tp_log_err("%s\n", __func__);
}

static void ilitek_i2c_late_resume(struct early_suspend *h)
{
	ilitek_i2c_resume(i2c.client);
	tp_log_info("%s\n", __func__);
}
#endif

void ilitek_i2c_irq_enable(void)
{
	if (i2c.irq_status == 0){
		i2c.irq_status = 1;
		enable_irq(i2c.client->irq);
		//tp_log_info("ilitek_i2c_irq_enable ok.\n");
	}
	else {
		//tp_log_info("no enable\n");
	}
}

void ilitek_i2c_irq_disable(void)
{
	if (i2c.irq_status == 1){
		i2c.irq_status = 0;
		disable_irq(i2c.client->irq);
		//tp_log_info("ilitek_i2c_irq_disable ok.\n");
	}
	else {
		//tp_log_info("no disable\n");
	}
}

static int set_command(struct i2c_client *client, uint8_t *cmd,
		int write_len, int delay, uint8_t *data, int read_len)
{
	int ret;
	struct i2c_msg msgs_send[] = {
		{.addr = client->addr, .flags = 0, .len = write_len, .buf = cmd,},
	};
	struct i2c_msg msgs_receive[] = {
		{.addr = client->addr, .flags = I2C_M_RD, .len = read_len, .buf = data,}
	};
	ret = ilitek_i2c_transfer(client, msgs_send, 1);
	if(ret < 0)
	{
		tp_log_err("%s, i2c write error, ret = %d\n", __func__, ret);
	}
	if(delay > 1)
		msleep(delay);
	if(read_len > 0){
		ret = ilitek_i2c_transfer(client, msgs_receive, 1);
		if(ret < 0)
		{
			tp_log_err("%s, i2c read error, ret = %d\n", __func__, ret);
		}
	}
	return ret;
}

int ilitek_i2c_read_tp_info( void)
{
	unsigned char buf[64] = {0};
	int i = 0;

	//read driver version
	//tp_log_info("%s, driver version:%d.%d.%d\n", __func__, driver_information[0], driver_information[1], driver_information[2]);
	//read firmware version

	buf[0] = ILITEK_TP_CMD_READ_DATA_CONTROL;
	buf[1] = ILITEK_TP_CMD_GET_FIRMWARE_VERSION;
	if(set_command(i2c.client, buf, 2, 10, buf, 0) < 0)
	{
		//tpd_load_status = -1;
		return -1;
	}
	buf[0] = ILITEK_TP_CMD_GET_FIRMWARE_VERSION;
	if(set_command(i2c.client, buf, 1, 10, buf, 3) < 0)
	{
		return -1;
	}
	tp_log_info("%s, firmware version:%d.%d.%d\n", __func__, buf[0], buf[1], buf[2]);
	i2c.firmware_ver[0] = 0;
	for(i = 1; i < 4; i++)
	{
		i2c.firmware_ver[i] = buf[i - 1];
	}
	//read protocol version
	buf[0] = ILITEK_TP_CMD_READ_DATA_CONTROL;
	buf[1] = ILITEK_TP_CMD_GET_PROTOCOL_VERSION;
	if(set_command(i2c.client, buf, 2, 10, buf, 0) < 0)
	{
		return -1;
	}

	buf[0] = ILITEK_TP_CMD_GET_PROTOCOL_VERSION;
	if(set_command(i2c.client, buf, 1, 10, buf, 2) < 0)
	{
		return -1;
	}
	i2c.protocol_ver = (((int)buf[0]) << 8) + buf[1];
	tp_log_info("%s, protocol version:%d.%d\n", __func__, buf[0], buf[1]);
	//read touch resolution
	buf[0] = ILITEK_TP_CMD_READ_DATA_CONTROL;
	buf[1] = 0x20;
	if(set_command(i2c.client, buf, 2, 10, buf, 0) < 0)
	{
		return -1;
	}
	buf[0] = 0x20;
	if(set_command(i2c.client, buf, 1, 10, buf, 10) < 0)
	{
		return -1;
	}
	//calculate the resolution for x and y direction
	i2c.max_x = buf[2];
	i2c.max_x+= ((int)buf[3]) * 256;
	if (JUDGE_ZERO == i2c.max_x)
	{
		i2c.max_x = MAX_X;
	}
	i2c.max_y = buf[4];
	i2c.max_y+= ((int)buf[5]) * 256;
	if (JUDGE_ZERO == i2c.max_y)
	{
		i2c.max_y = MAX_Y;
	}
	i2c.min_x = buf[0];
	i2c.min_y = buf[1];
	i2c.x_ch = buf[6];
	if (JUDGE_ZERO == i2c.x_ch)
	{
		i2c.x_ch = X_CHANNEL_NUM;
	}
	i2c.y_ch = buf[7];
	if (JUDGE_ZERO == i2c.y_ch)
	{
		i2c.y_ch = Y_CHANNEL_NUM;
	}
	//maximum touch point
	i2c.max_tp = buf[8];
	if (JUDGE_ZERO == i2c.max_tp)
	{
		i2c.max_tp = MAX_TOUCH_POINT;
	}
	//key count
	i2c.keycount = buf[9];

	buf[0] = ILITEK_TP_CMD_READ_DATA_CONTROL;
	buf[1] = 0xF8;
	if(set_command(i2c.client, buf, 2, 10, buf, 0) < 0)
	{
		return -1;
	}
	buf[0] = 0xF8;
	if(set_command(i2c.client, buf, 1, 10, buf, 1) < 0)
	{
		return -1;
	}
	if (buf[0] != 0) {
		i2c.x_allnode_ch = buf[0];
	} else {
		i2c.x_allnode_ch = i2c.x_ch;
	}
	tp_log_info("%s, min_x: %d, max_x: %d, min_y: %d, max_y: %d, ch_x: %d, ch_y: %d, max_tp: %d, key_count: %d x_allnode_ch:%d\n"
			, __func__, i2c.min_x, i2c.max_x, i2c.min_y, i2c.max_y, i2c.x_ch, i2c.y_ch, i2c.max_tp, i2c.keycount,i2c.x_allnode_ch);
	return 0;
}

static int ilitek_request_io_port(struct i2c_client *client)
{
	int err = 0;

	struct device *dev = &(client->dev);
	struct device_node *np = dev->of_node;

	i2c.reset_gpio = of_get_named_gpio_flags(np, "chipone,reset-gpio",0, &i2c.reset_gpio_flags);
	if (i2c.reset_gpio < 0)
	{
		tp_log_err("rst_gpio = %d\n",i2c.reset_gpio);
		return i2c.reset_gpio;
	}

	i2c.irq_gpio = of_get_named_gpio_flags(np, "chipone,irq-gpio",0, &i2c.irq_gpio_flags);
	if (i2c.irq_gpio < 0)
	{
		tp_log_err("irq_gpio = %d\n",i2c.irq_gpio);
		return i2c.irq_gpio;
	}
	
	if (of_find_property(np, "chipone,vci-gpio", NULL)) {
		i2c.vci_gpio = of_get_named_gpio_flags(np, "chipone,vci-gpio", 0, NULL);
		err = gpio_request(i2c.vci_gpio, "vci-gpio");
		if (err){
			tp_log_err("Failed to request vci_gpio \n");
			return err;
		}

		err = gpio_direction_output(i2c.vci_gpio, 1);
		if (err) {
			tp_log_err("Failed to direction output vci_gpio GPIO err");
			return err;
		}
	}else {
		i2c.vci_gpio = -1;
	}
	
	err= gpio_request(i2c.reset_gpio, "reset-gpio");
	if (err) {
		tp_log_err("Failed to request reset_gpio \n");
		return err;
	}
	err = gpio_direction_output(i2c.reset_gpio, 0);
	if (err) {
		tp_log_err("Failed to direction output rest GPIO err");
		return err;
	}
	msleep(20);
	gpio_set_value_cansleep(i2c.reset_gpio, 1);

	err = gpio_request(i2c.irq_gpio, "irq-gpio"); 
	if (err < 0)
	{
		tp_log_err("Failed to request irq_gpio\n");
		return err;
	}
	err = gpio_direction_input(i2c.irq_gpio);
	if (err) {
		tp_log_err("Failed to direction output rest GPIO err");
		return err;
	}
	i2c.client->irq  = gpio_to_irq(i2c.irq_gpio);
	
	return err;
}

static int ilitek_power_on(void)
{
	char const *power_pin_vdd = NULL;
	char const *power_pin_vbus = NULL;
	struct regulator *vdd_synaptics = NULL;
	struct regulator *vbus_synaptics = NULL;
	int rc;
	struct i2c_client *client = i2c.client;
	struct device *dev = &(client->dev);
	
	/*get the power name*/
	rc = of_property_read_string(dev->of_node,"ilitek,vdd", &power_pin_vdd);
	if (rc) {
		tp_log_warning("%s(%d): OF error vdd rc=%d\n", __func__, __LINE__, rc);
	}
	
	rc = of_property_read_string(dev->of_node,"ilitek,vbus", &power_pin_vbus);
	if (rc) {
		tp_log_warning("%s(%d): OF error vbus rc=%d\n", __func__, __LINE__, rc);
	}
	
	/* VDD power on */
	if(power_pin_vdd)
	{
		vdd_synaptics = regulator_get(dev, power_pin_vdd);
		if (IS_ERR(vdd_synaptics)) {
			tp_log_err("%s: vdd_ilitek regulator get fail, rc=%d\n", __func__, rc);
			return  -EINVAL;
		}
	
		syn_power_vdd = vdd_synaptics;
	
		rc = regulator_set_voltage(vdd_synaptics, 2850000, 2850000);
		if(rc < 0){
			tp_log_err( "%s: vdd_ilitek regulator set fail, rc=%d\n", __func__, rc);
			return  -EINVAL;
		}
	
		rc = regulator_enable(vdd_synaptics);
		if (rc < 0) {
			tp_log_err( "%s: vdd_ilitek regulator enable fail, rc=%d\n", __func__, rc);
			return -EINVAL;
		}
	}
	
	/* Modify JDI tp/lcd power on/off to reduce power consumption */
	if(power_pin_vbus)
	{
		vbus_synaptics = regulator_get(dev, power_pin_vbus);
		if (IS_ERR(vbus_synaptics)) {
			tp_log_err( "%s: vbus_ilitek regulator get fail, rc=%d\n", __func__, rc);
			return -EINVAL;
		}
	
		syn_power_vbus = vbus_synaptics;
	
		rc = regulator_set_voltage(vbus_synaptics,1800000,1800000);
		if(rc < 0){
			tp_log_err( "%s: vbus_ilitek regulator set fail, rc=%d\n", __func__, rc);
			return -EINVAL;
		}
	
		rc = regulator_enable(vbus_synaptics);
		if (rc < 0) {
			tp_log_err( "%s: vbus_ilitek regulator enable fail, rc=%d\n", __func__, rc);
			return -EINVAL;
		}
	}
	return 0;
}

int ilitek_i2c_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int ret = 0;
#ifdef GESTURE
	uint8_t cmd[2] = {0};
	struct i2c_msg msgs_cmd[] = {
		{.addr = client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	system_resume_ilitek = 0;
	#ifdef ILI_UPDATE_FW
	if (update_wait_flag == 1) {
		tp_log_info("%s, ilitek waiting update so return\n", __func__);
		return 0;
	}
	#endif
	if(i2c.firmware_updating) {
		tp_log_info("%s: tp fw is updating,return\n", __func__);
		return 0;
	}
	tp_log_info("Enter ilitek_i2c_suspend 0x01 0x00, 0x0A 0x01\n");
	cmd[0] = 0x01;
	cmd[1] = 0x00;
	ret = ilitek_i2c_transfer(client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s, 0x01 0x00 set tp suspend err, ret %d\n", __func__, ret);
	}
	msleep(10);
	cmd[0] = 0x0A;
	cmd[1] = 0x01;
	ret = ilitek_i2c_transfer(client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s, 0x0A 0x01 set tp suspend err, ret %d\n", __func__, ret);
	}
	enable_irq_wake(i2c.client->irq); 
#else
	uint8_t cmd[2] = {0};
	struct i2c_msg msgs_cmd[] = {
		{.addr = client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	#ifdef ILI_UPDATE_FW
	if (update_wait_flag == 1) {
		tp_log_info("%s, ilitek waiting update so return\n", __func__);
		return 0;
	}
	#endif
	tp_log_info("Enter %s i2c.valid_irq_request=%d i2c.reset_request_success=%d\n",__FUNCTION__,i2c.valid_irq_request,i2c.reset_request_success);
	if(i2c.valid_irq_request != 0){
		ilitek_i2c_irq_disable();
	}
	else{
		i2c.stop_polling = 1;
		tp_log_info("%s, stop i2c thread polling\n", __func__);
	}

	if(i2c.reset_request_success){
		cmd[0] = 0x01;
		cmd[1] = 0x00;
		ret = ilitek_i2c_transfer(client, msgs_cmd, 1);
		if(ret < 0){
			tp_log_err("%s, 0x01 0x00 set tp suspend err, ret %d\n", __func__, ret);
		}
		msleep(10);
		cmd[0] = ILITEK_TP_CMD_SLEEP;
		cmd[1] = 0x00;
		ret = ilitek_i2c_transfer(client, msgs_cmd, 1);
		if(ret < 0){
			tp_log_err("%s, 0x02 0x00 set tp suspend err, ret %d\n", __func__, ret);
		}
	}
#endif

#if 1		
#ifdef TOUCH_PROTOCOL_B
	for(ret = 0; ret < i2c.max_tp; ret++)
	{
		if(i2c.touchinfo[ret].flag == 1)
		{
			tp_log_info("ilitek release point i = %d\n", ret);
			input_mt_slot(i2c.input_dev, ret);
			input_mt_report_slot_state(i2c.input_dev, MT_TOOL_FINGER, false);
		}
		i2c.touchinfo[ret].flag = 0;
	}
#else
	if(i2c.touch_flag == 1)
	{
		i2c.touch_flag = 0;
		//tp_log_info("Release, ID=%02X, X=%04d, Y=%04d\n", buf[0] & 0x3F, x, y);
		input_report_key(i2c.input_dev, BTN_TOUCH,	0);
		input_mt_sync(i2c.input_dev);
	}
#endif
		//tp_log_info("%s,ret=%d\n",__func__,ret);
#ifdef TOUCH_PROTOCOL_B
	input_mt_report_pointer_emulation(i2c.input_dev, true);
#endif
	input_sync(i2c.input_dev);
#endif
	return 0;
}

int ilitek_i2c_resume(struct i2c_client *client)
{
#ifdef GESTURE
	//gesture_flag = 0;
	system_resume_ilitek = 1;
	//disable_irq_wake(i2c.client->irq); 
#endif
	#ifdef ILI_UPDATE_FW
	if (update_wait_flag == 1) {
		tp_log_info("%s,ilitek_i2c_resume  ilitek waiting update so return\n", __func__);
		return 0;
	}
	#endif
	if(i2c.firmware_updating) {
		tp_log_info("%s: tp fw is updating,return\n", __func__);
		return 0;
	}
	tp_log_info("ENTER %s i2c.reset_request_success = %x i2c.valid_irq_request=%d\n", __FUNCTION__,i2c.reset_request_success,i2c.valid_irq_request);
	if(i2c.reset_request_success)
	{
		ilitek_reset(i2c.reset_gpio);
	}

	if(i2c.valid_irq_request != 0){
		ilitek_i2c_irq_enable();
	}
	else{
		i2c.stop_polling = 0;
		tp_log_info("%s, start i2c thread polling\n", __func__);
	}
	return 0;
}

static int ilitek_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int retval;
	int ret = 0;
	#ifdef ILI_UPDATE_FW
	struct task_struct *thread_update = NULL;
	#endif
	struct ilitek_platform_data *platform_data = client->dev.platform_data;
	
	tp_log_info("Enter ilitek_i2c_probe \n");
	if(!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)){
		tp_log_err("%s, I2C_FUNC_I2C not support\n", __func__);
		return -EIO;;
	}

	if (TOUCH_DETECTED == touch_hw_data.read_touch_probe_flag()) {
		tp_log_err("%s: other TP module have detected\n", __func__);
		return -EINVAL;
	}

	#ifdef TOOL
	memset(&dev_ilitek, 0, sizeof(struct dev_data));
	#endif
#if  0
	if (client->dev.of_node) {
		platform_data = devm_kzalloc(&client->dev,sizeof(struct ilitek_platform_data), GFP_KERNEL);
		if (!platform_data) {
			tp_log_err("%s: Failed to allocate memory for pdata\n", __func__);
			return -ENOMEM;
		}

		retval = synaptics_rmi4_parse_dt(&client->dev, platform_data);
		if (retval) {
			tp_log_info("%s %d:parse dt fail, retval=%d\n", __func__, __LINE__, retval);
			return retval;
		}
	}
	
	if (!platform_data) {
		tp_log_err("%s: No platform data found\n", __func__);
		return -EINVAL;
	}
#endif
	memset(&i2c, 0, sizeof(struct i2c_data));
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 37)
	init_MUTEX(&i2c.wr_sem);
#else
	sema_init(&i2c.wr_sem,1);
#endif
	i2c.wr_sem.count = 1;
	i2c.report_status = 1;
	i2c.firmware_updating = false;
	i2c.client = client;

	retval = ilitek_power_on();
	if (retval) {
		tp_log_err("%s: power on fail, retval=%d\n", __func__, retval);
		goto err_regulator;
	}

	retval = ilitek_request_io_port(client);
	if (retval){
		tp_log_err("io error");
		goto err_read_tp_info;
	}
	
	i2c.valid_i2c_register = 1;

	retval = ilitek_request_init_reset();
	if(retval < 0){
		tp_log_err("ilitek request reset err\n");
		i2c.reset_request_success = 0;
	}
	else{
		tp_log_info("ilitek request reset success\n");
		i2c.reset_request_success = 1;
	}
	msleep(200);

	retval = ilitek_i2c_read_tp_info();
	if(retval < 0){
		tp_log_err("ilitek read tp info fail\n");
		goto err_read_tp_info;
	}
	i2c.input_dev = input_allocate_device();
	if(i2c.input_dev == NULL){
		tp_log_err("%s, allocate input device, error\n", __func__);
		goto err_read_tp_info;
	}

	tp_log_info("%s, register input device, success\n", __func__);
	i2c.valid_input_register = 1;
	
#if defined(CONFIG_FB)
		i2c.fb_notif.notifier_call = fb_notifier_callback;
		retval = fb_register_client(&i2c.fb_notif);
		if (retval) {
			dev_err(&i2c.client->dev, "Unable to register fb_notifier: %d\n",ret);
			goto err_register_device;
		}

#elif defined(CONFIG_HAS_EARLYSUSPEND)
		i2c.early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
		i2c.early_suspend.suspend = ilitek_i2c_early_suspend;
		i2c.early_suspend.resume = ilitek_i2c_late_resume;
		register_early_suspend(&i2c.early_suspend);
#endif
	
#ifndef POLLING_MODE

		tp_log_info("%s, IRQ: 0x%X\n", __func__, (i2c.client->irq));

		if(i2c.client->irq != 0 ){ 
			i2c.irq_work_queue = create_singlethread_workqueue("ilitek_i2c_irq_queue");
			if(i2c.irq_work_queue){
				INIT_WORK(&i2c.irq_work, ilitek_i2c_irq_work_queue_func);
#ifdef CLOCK_INTERRUPT
				if(Request_IRQ()){
					tp_log_err("%s, request irq, error\n", __func__);
					goto err_register_device;
				}else{
					i2c.valid_irq_request = 1;
					i2c.irq_status = 1;
					tp_log_err("%s, request irq, success\n", __func__);
				}
#else
				init_timer(&i2c.timer);
				i2c.timer.data = (unsigned long)&i2c;
				i2c.timer.function = ilitek_i2c_timer;
				if(Request_IRQ()){
					tp_log_err("%s, request irq, error\n", __func__);
					goto err_register_device;
				}
				else{
					i2c.valid_irq_request = 1;
					i2c.irq_status = 1;
					tp_log_err("%s, request irq, success\n", __func__);
				}
#endif
			}
		}
		else
#endif
			{
#ifdef POLLING_MODE
	seting_polling_mode:
#endif
				tp_log_info( "%s,i2c.client->irq = %d , setting polling mode \n", __func__,(i2c.client->irq));
				i2c.stop_polling = 0;
				i2c.thread = kthread_create(ilitek_i2c_polling_thread, NULL, "ilitek_i2c_thread");
				if(i2c.thread == (struct task_struct*)ERR_PTR){
					i2c.thread = NULL;
					tp_log_err("%s, kthread create, error\n", __func__);
					goto err_create_thread;
				}
				else{
					i2c.set_polling_mode = 1;
					wake_up_process(i2c.thread);
				}
			}
#ifdef POLLING_MODE
			if(i2c.set_polling_mode != 1){
				goto seting_polling_mode;
			}
#endif

#ifdef REPORT_THREAD
	i2c.thread = kthread_run(ilitek_i2c_touchevent_thread, NULL, "ilitek_i2c_thread");
	if(i2c.thread == (struct task_struct*)ERR_PTR){
		i2c.thread = NULL;
		tp_log_err("%s, kthread create, error\n", __func__);
		goto err_run_thread;
	}
#endif

#ifdef GESTURE
	system_resume_ilitek = 1;
	input_set_capability(i2c.input_dev, EV_KEY, KEY_POWER);
	input_set_capability(i2c.input_dev, EV_KEY, KEY_W);
	input_set_capability(i2c.input_dev, EV_KEY, KEY_O);
	input_set_capability(i2c.input_dev, EV_KEY, KEY_C);
	input_set_capability(i2c.input_dev, EV_KEY, KEY_E);
	input_set_capability(i2c.input_dev, EV_KEY, KEY_M);
#endif

#ifdef TOOL
	ret = create_tool_node();
#endif

#ifdef SENSOR_TEST
#ifdef SYS_ATTR_FILE
	ilitek_touch_node_init();
#endif
#endif

#ifdef HALL_CHECK
	ilitek_hall_check_hw_init();
#endif
	Report_Flag=0;
	set_tp_type(ILITEK_JDI_MODULE);
#ifdef ILI_UPDATE_FW
	thread_update= kthread_run(ilitek_i2c_update_thread, NULL, "ilitek_i2c_updatethread");
	if(thread_update == (struct task_struct*)ERR_PTR){
		thread_update = NULL;
		tp_log_err(ILITEK_ERROR_LEVEL "%s,thread_update kthread create, error\n", __func__);
	}
#endif
	return retval;
err_run_thread:
	if(i2c.irq_work_queue){
		destroy_workqueue(i2c.irq_work_queue);
	}
err_create_thread:
	free_irq(i2c.irq,&i2c);
err_register_device:
	if (NULL != i2c.input_dev){
		input_unregister_device(i2c.input_dev);
		i2c.input_dev = NULL;
	}
err_read_tp_info:
	if(i2c.reset_gpio){
		gpio_free(i2c.reset_gpio);
	}
	if(i2c.irq_gpio){
		gpio_free(i2c.irq_gpio);
	}
	if(i2c.vci_gpio){
		gpio_free(i2c.vci_gpio);
	}
err_regulator:
	if (client->dev.of_node) {
		if (platform_data) {
			//devm_kfree(&client->dev, platform_data);
		}
	}

	tp_log_err("%s: ilitek probe fail\n", __func__);
	return retval;
}

static int ilitek_i2c_remove(struct i2c_client *client)
{
	tp_log_info( "%s\n", __func__);
	i2c.stop_polling = 1;
#if defined(CONFIG_FB)
	if (fb_unregister_client(&i2c.fb_notif))
		dev_err(&client->dev, "Error occurred while unregistering fb_notifier.\n");
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	unregister_early_suspend(&i2c.early_suspend);
#endif
	if(i2c.client->irq != 0){
		if(i2c.valid_irq_request != 0){
			free_irq(i2c.client->irq, &i2c);
			tp_log_info("%s, free irq\n", __func__);
			if(i2c.irq_work_queue){
				destroy_workqueue(i2c.irq_work_queue);
				tp_log_info("%s, destory work queue\n", __func__);
			}
		}
#ifdef REPORT_THREAD
		if(i2c.thread != NULL){
			kthread_stop(i2c.thread);
			tp_log_info("%s, stop i2c thread\n", __func__);
		}
#endif
	}
	else{
		if(i2c.thread != NULL){
			kthread_stop(i2c.thread);
			tp_log_info("%s, stop i2c thread\n", __func__);
		}
	}
	if(i2c.valid_input_register != 0){
		input_unregister_device(i2c.input_dev);
		tp_log_info("%s, unregister i2c input device\n", __func__);
	}

	#ifdef TOOL
		remove_tool_node();
	#endif
	
	#ifdef SENSOR_TEST
	#ifdef SYS_ATTR_FILE
		ilitek_touch_node_deinit();
	#endif
	#endif
	#ifdef HALL_CHECK
		ilitek_hall_check_hw_deinit();
	#endif
	tp_log_info("%s\n", __func__);
	return 0;
}

static int ilitek_init(void)
{
	int ret = 0;
	tp_log_info("%s\n", __func__);
	ret = i2c_add_driver(&ili2120_ts_driver);
	if(ret != 0)
	{
		tp_log_err("%s, add i2c device, error\n", __func__);
		return ret;
	}
	return 0;
}

static void ilitek_exit(void)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&i2c.early_suspend);
#endif
	if(i2c.client->irq != 0){
		if(i2c.valid_irq_request != 0){
			free_irq(i2c.client->irq, &i2c);
			tp_log_info("%s, free irq\n", __func__);
			if(i2c.irq_work_queue){
				destroy_workqueue(i2c.irq_work_queue);
				tp_log_info("%s, destory work queue\n", __func__);
			}
		}
		#ifdef REPORT_THREAD
		if(i2c.thread != NULL){
			kthread_stop(i2c.thread);
			tp_log_info("%s, stop i2c thread\n", __func__);
		}
		#endif
	}
	else{
		if(i2c.thread != NULL){
			kthread_stop(i2c.thread);
			tp_log_info("%s, stop i2c thread\n", __func__);
		}
	}
	if(i2c.valid_i2c_register != 0){
		i2c_del_driver(&ili2120_ts_driver);
		tp_log_info("%s, delete i2c driver\n", __func__);
	}
	if(i2c.valid_input_register != 0){
		input_unregister_device(i2c.input_dev);
		tp_log_info("%s, unregister i2c input device\n", __func__);
	}

	#ifdef TOOL
	remove_tool_node();
	#endif
	tp_log_info("%s\n", __func__);
}

module_init(ilitek_init);
module_exit(ilitek_exit);
