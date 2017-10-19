#include "ilitek_ts.h"
#ifdef SENSOR_TEST
#define SYS_ATTR_FILE
#ifdef SYS_ATTR_FILE
struct kobject *android_touch_kobj;
void ilitek_touch_node_deinit(void);
void ilitek_touch_node_init(void);
extern struct i2c_data i2c;
#endif
#endif
extern void ilitek_reset(int);
#ifdef SENSOR_TEST
static int i2c_connect = 1;
static int microshorttestwindow = 7, short_test_result = 0;
static int opentestthrehold = 3200, open_test_result = 0;
static int allnodetestymax = 2500, allnodetestymin = 1500, allnode_test_result = 0;
static int allnodetestymax_self = 2500, allnodetestymin_self = 1500, allnode_self_test_result = 0;

static int ilitek_ready_into_test(void) {
	int ret = 0;
	uint8_t cmd[2] = {0};
	uint8_t buf[32] = {0};
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	struct i2c_msg msgs_ret[] = {
		{.addr = i2c.client->addr, .flags = I2C_M_RD, .len = 3, .buf = buf,}
	};

	cmd[0] = 0x10;
	msgs_cmd[0].len = 1;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		i2c_connect = 0;
		tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		return ret;
	}
	msgs_ret[0].len = 3;
	ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
	if(ret < 0){
		tp_log_err("%s, i2c read error, ret %d\n", __func__, ret);
		return ret;
	}
	if (buf[1] < 0x80) {
		tp_log_info("ilitek_ready_into_test IC is not ready\n");
	}
	cmd[0] = 0xF6;
	cmd[1] = 0x13;
	msgs_cmd[0].len = 2;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		return ret;
	}

	cmd[0] = 0x13;
	msgs_cmd[0].len = 1;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		return ret;
	}
	msgs_ret[0].len = 2;
	ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
	if(ret < 0){
		tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		return ret;
	}
	return 0;
}

static int ilitek_into_testmode(void) {
	int ret = 0;
	uint8_t cmd[2] = {0};
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	cmd[0] = 0xF0;
	cmd[1] = 0x01;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s,  err, ret %d\n", __func__, ret);
		return ret;
	}
	return 0;
}
static int ilitek_into_normalmode(void) {
	int ret = 0;
	uint8_t cmd[2] = {0};
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	cmd[0] = 0xF0;
	cmd[1] = 0x00;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s,  err, ret %d\n", __func__, ret);
		return ret;
	}
	return 0;
}
#ifdef SYS_ATTR_FILE

static int ilitek_short_test(int *short_data1, int *short_data2) {
	int ret = 0, newMaxSize = 32, i = 0, j = 0, index = 0;
	uint8_t cmd[4] = {0};
	uint8_t * buf_recv = kzalloc(sizeof(uint8_t) * i2c.y_ch * i2c.x_ch * 2 + 32, GFP_KERNEL);

	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	struct i2c_msg msgs_ret[] = {
		{.addr = i2c.client->addr, .flags = I2C_M_RD, .len = 3, .buf = buf_recv,}
	};

	int test_32 = (i2c.x_ch * 2) / (newMaxSize - 2);
	if ((i2c.x_ch * 2) % (newMaxSize - 2) != 0) {
		test_32 += 1;
	}

	short_test_result = 0;
	//ilitek_i2c_irq_disable();
	//ret = ilitek_ready_into_test();
	//ret = ilitek_into_testmode();
#if 1
		cmd[0] = 0xF1;
		cmd[1] = 0x04;
		cmd[2] = 0x00;
		msgs_cmd[0].len = 3;
		ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
		if(ret < 0){
			tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
			//return ret;
		}
		mdelay(350);
		cmd[0] = 0xF6;
		cmd[1] = 0xF2;
		msgs_cmd[0].len = 2;
		ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
		if(ret < 0){
			tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
			//return ret;
		}
		cmd[0] = 0xF2;
		msgs_cmd[0].len = 1;
		ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
		if(ret < 0){
			tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
			//return ret;
		}
		for(i = 0; i < test_32; i++){
			if ((i2c.x_ch * 2)%(newMaxSize - 2) != 0 && i == test_32 - 1) {
				msgs_ret[0].len = (i2c.x_ch * 2)%(newMaxSize - 2) + 2;
				msgs_ret[0].buf = buf_recv + newMaxSize*i;
				ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
				if(ret < 0){
					tp_log_err("%s:%d err,2c read error ret %d\n", __func__,__LINE__,ret);
				}
			}
			else {
				msgs_ret[0].len = newMaxSize;
				msgs_ret[0].buf = buf_recv + newMaxSize*i;
				ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
				if(ret < 0){
					tp_log_err("%s:%d err,2c read error ret %d\n", __func__,__LINE__,ret);
					//return ret;
				}
			}
			//tp_log_info("buf_recv 0x%x, 0x%x\n", buf_recv[newMaxSize*i], buf_recv[newMaxSize*i + 1]);
		}
		j = 0;
		for(i = 0; i < test_32; i++) {
			if (j == i2c.x_ch * 2) {
				break;
			}
			for(index = 2; index < newMaxSize; index++) {
				if (j < i2c.x_ch) {
					short_data1[j] = buf_recv[i * newMaxSize + index];
				}
				else {
					short_data2[j - i2c.x_ch] = buf_recv[i * newMaxSize + index];
				}
				j++;
				if(j % i2c.x_ch == 0) {
				}
				if (j == i2c.x_ch * 2) {
					break;
				}
			}
		}
		for (i = 0; i < i2c.x_ch; i++) {
			if (abs(short_data1[i] - short_data2[i]) > microshorttestwindow) {
				short_test_result = -1;
			}
		}
#endif
	//ret = ilitek_into_normalmode();
	//ilitek_i2c_irq_enable();
	if (buf_recv) {
		kfree(buf_recv);
		buf_recv = NULL;
	}
	return short_test_result;
}

static int ilitek_open_test(int *open_data) {
	int ret = 0, newMaxSize = 32, i = 0, j = 0, index = 0;
	uint8_t cmd[4] = {0};
	uint8_t * buf_recv = kzalloc(sizeof(uint8_t) * i2c.y_ch * i2c.x_ch * 2 + 32, GFP_KERNEL);

	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	struct i2c_msg msgs_ret[] = {
		{.addr = i2c.client->addr, .flags = I2C_M_RD, .len = 3, .buf = buf_recv,}
	};

	int test_32 = (i2c.y_ch * i2c.x_ch * 2) / (newMaxSize - 2);
	if ((i2c.y_ch * i2c.x_ch * 2) % (newMaxSize - 2) != 0) {
		test_32 += 1;
	}

	open_test_result = 0;
	//ilitek_i2c_irq_disable();
	//ret = ilitek_ready_into_test();
	//ret = ilitek_into_testmode();
#if 1
	cmd[0] = 0xF1;
	cmd[1] = 0x06;
	cmd[2] = 0x00;
	msgs_cmd[0].len = 3;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		//return ret;
	}
	mdelay(350);
	cmd[0] = 0xF6;
	cmd[1] = 0xF2;
	msgs_cmd[0].len = 2;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		//return ret;
	}
	cmd[0] = 0xF2;
	msgs_cmd[0].len = 1;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		//return ret;
	}
	tp_log_info("ilitek_open_test test_32 = %d\n", test_32);
	for(i = 0; i < test_32; i++){
		if ((i2c.y_ch * i2c.x_ch * 2)%(newMaxSize - 2) != 0 && i == test_32 - 1) {
			msgs_ret[0].len = (i2c.y_ch * i2c.x_ch * 2)%(newMaxSize - 2) + 2;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				tp_log_err("%s:%d err,i2c read error ret %d\n", __func__,__LINE__,ret);
			}
		}
		else {
			msgs_ret[0].len = newMaxSize;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				tp_log_err("%s:%d err,i2c read error ret %d\n", __func__,__LINE__,ret);
			}
		}
	}
	j = 0;
	for(i = 0; i < test_32; i++) {
		if (j == i2c.y_ch * i2c.x_ch) {
			break;
		}
		for(index = 2; index < newMaxSize; index += 2) {
			open_data[j] = ((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]);
			if (((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]) < opentestthrehold) {
				open_test_result = -1;
			}
			j++;
			if(j % i2c.x_ch == 0) {
			}
			if (j == i2c.y_ch * i2c.x_ch) {
				break;
			}
		}
	}
#endif
	//ret = ilitek_into_normalmode();
	//ilitek_i2c_irq_enable();
	if (buf_recv) {
		kfree(buf_recv);
		buf_recv = NULL;
	}
	return open_test_result;
}


static int ilitek_allnode_test(int *allnode_data) {
	int ret = 0, newMaxSize = 32, i = 0, j = 0, index = 0;
	uint8_t cmd[4] = {0};
	uint8_t * buf_recv = kzalloc(sizeof(uint8_t) * i2c.y_ch * i2c.x_allnode_ch * 2 + 32, GFP_KERNEL);

	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	struct i2c_msg msgs_ret[] = {
		{.addr = i2c.client->addr, .flags = I2C_M_RD, .len = 3, .buf = buf_recv,}
	};

	int test_32 = (i2c.y_ch * i2c.x_allnode_ch * 2) / (newMaxSize - 2);
	if ((i2c.y_ch * i2c.x_allnode_ch * 2) % (newMaxSize - 2) != 0) {
		test_32 += 1;
	}
	tp_log_info("ilitek_allnode_test test_32 = %d\n", test_32);
	allnode_test_result = 0;
	//ilitek_i2c_irq_disable();
	//ret = ilitek_ready_into_test();
	//ret = ilitek_into_testmode();
	//test_32 = (i2c.y_ch * i2c.x_allnode_ch * 2) / (newMaxSize - 2);
	cmd[0] = 0xF1;
	cmd[1] = 0x08;
	cmd[2] = 0x00;
	msgs_cmd[0].len = 3;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err,i2c  error ret %d\n", __func__,__LINE__,ret);
		//return ret;
	}
	mdelay(350);
	cmd[0] = 0xF6;
	cmd[1] = 0xF2;
	msgs_cmd[0].len = 2;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err,i2c  error ret %d\n", __func__,__LINE__,ret);
		//return ret;
	}
	cmd[0] = 0xF2;
	msgs_cmd[0].len = 1;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err,i2c  error ret %d\n", __func__,__LINE__,ret);
		//return ret;
	}
	for(i = 0; i < test_32; i++){
		if ((i2c.y_ch * i2c.x_allnode_ch * 2)%(newMaxSize - 2) != 0 && i == test_32 - 1) {
			msgs_ret[0].len = (i2c.y_ch * i2c.x_allnode_ch * 2)%(newMaxSize - 2) + 2;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				tp_log_err("%s:%d err,i2c read error ret %d\n", __func__,__LINE__,ret);
				//return ret;
			}
		}
		else {
			msgs_ret[0].len = newMaxSize;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				tp_log_err("%s:%d err,i2c read error ret %d\n", __func__,__LINE__,ret);
				//return ret;
			}
		}
	}
	j = 0;
	for(i = 0; i < test_32; i++) {
		if (j == i2c.y_ch * i2c.x_allnode_ch) {
			break;
		}
		for(index = 2; index < newMaxSize; index += 2) {
			allnode_data[j] = ((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]);
			if (((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]) < allnodetestymin || 
				((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]) > allnodetestymax) {
				allnode_test_result = -1;
			}
			j++;
			if(j % i2c.x_allnode_ch == 0) {
			}
			if (j == i2c.y_ch * i2c.x_allnode_ch) {
				break;
			}
		}
	}
	//ret = ilitek_into_normalmode();
	//ilitek_i2c_irq_enable();
	if (buf_recv) {
		kfree(buf_recv);
		buf_recv = NULL;
	}
	return allnode_test_result;
}


static int ilitek_allnode_self_test(int *allnode_self_data) {
	int ret = 0, newMaxSize = 32, i = 0, j = 0, index = 0;
	uint8_t cmd[4] = {0};
	uint8_t * buf_recv = kzalloc(sizeof(uint8_t) * i2c.y_ch * i2c.x_ch * 2 + 32, GFP_KERNEL);

	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	struct i2c_msg msgs_ret[] = {
		{.addr = i2c.client->addr, .flags = I2C_M_RD, .len = 3, .buf = buf_recv,}
	};

	int test_32 = (i2c.y_ch * i2c.x_ch * 2) / (newMaxSize - 2);
	if ((i2c.y_ch * i2c.x_ch * 2) % (newMaxSize - 2) != 0) {
		test_32 += 1;
	}

	allnode_self_test_result = 0;
	//ilitek_i2c_irq_disable();
	//ret = ilitek_ready_into_test();
	//ret = ilitek_into_testmode();
	test_32 = (i2c.x_ch * 2) / (newMaxSize - 2);
	cmd[0] = 0xF1;
	cmd[1] = 0x0B;
	cmd[2] = 0x00;
	msgs_cmd[0].len = 3;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err,i2c  error ret %d\n", __func__,__LINE__,ret);
	}
	mdelay(350);
	cmd[0] = 0xF6;
	cmd[1] = 0xF2;
	msgs_cmd[0].len = 2;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err,i2c  error ret %d\n", __func__,__LINE__,ret);
	}
	cmd[0] = 0xF2;
	msgs_cmd[0].len = 1;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err,i2c  error ret %d\n", __func__,__LINE__,ret);
	}
	for(i = 0; i < test_32; i++){
		if ((i2c.x_ch * 2)%(newMaxSize - 2) != 0 && i == test_32 - 1) {
			msgs_ret[0].len = (i2c.x_ch * 2)%(newMaxSize - 2) + 2;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				tp_log_err("%s:%d err,i2c read error ret %d\n", __func__,__LINE__,ret);
			}
		}
		else {
			msgs_ret[0].len = newMaxSize;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				tp_log_err("%s:%d err,i2c read error ret %d\n", __func__,__LINE__,ret);
			}
		}
	}
	j = 0;
	for(i = 0; i < test_32; i++) {
		if (j == i2c.x_ch) {
			break;
		}
		for(index = 2; index < newMaxSize; index += 2) {
			allnode_self_data[j] = ((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]);
			if (((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]) < allnodetestymin_self|| 
				((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]) > allnodetestymax_self) {
				allnode_self_test_result = -1;
			}
			j++;
			if(j % i2c.x_ch == 0) {
			}
			if (j == i2c.x_ch) {
				break;
			}
		}
	}
	//ret = ilitek_into_normalmode();
	//ilitek_i2c_irq_enable();
	if (buf_recv) {
		kfree(buf_recv);
		buf_recv = NULL;
	}
	return allnode_self_test_result;
}

static ssize_t ilitek_open_test_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0, newMaxSize = 32, i = 0, j = 0, index = 0, len = 0;
	uint8_t cmd[4] = {0};
	uint8_t * buf_recv = kzalloc(sizeof(uint8_t) * i2c.y_ch * i2c.x_ch * 2 + 32, GFP_KERNEL);
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	struct i2c_msg msgs_ret[] = {
		{.addr = i2c.client->addr, .flags = I2C_M_RD, .len = 3, .buf = buf_recv,}
	};
	int test_32 = (i2c.y_ch * i2c.x_ch * 2) / (newMaxSize - 2);
	if (!buf_recv) {
		//tp_log_err("ilitek_all_test buf_recv kzalloc error\n");
	}
	if ((i2c.y_ch * i2c.x_ch * 2) % (newMaxSize - 2) != 0) {
		test_32 += 1;
	}
	ilitek_i2c_irq_disable();
	ret = ilitek_ready_into_test();
	ret = ilitek_into_testmode();
	cmd[0] = 0xF1;
	cmd[1] = 0x06;
	cmd[2] = 0x00;
	msgs_cmd[0].len = 3;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		//return ret;
	}
	mdelay(350);
	cmd[0] = 0xF6;
	cmd[1] = 0xF2;
	msgs_cmd[0].len = 2;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		//return ret;
	}
	cmd[0] = 0xF2;
	msgs_cmd[0].len = 1;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		//return ret;
	}
	tp_log_info("ilitek_open_test test_32 = %d\n", test_32);
	for(i = 0; i < test_32; i++){
		if ((i2c.y_ch * i2c.x_ch * 2)%(newMaxSize - 2) != 0 && i == test_32 - 1) {
			msgs_ret[0].len = (i2c.y_ch * i2c.x_ch * 2)%(newMaxSize - 2) + 2;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				tp_log_err("%s:%d, i2c read error, ret %d\n", __func__,__LINE__,ret);
				//return ret;
			}
		}
		else {
			msgs_ret[0].len = newMaxSize;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				tp_log_err("%s:%d, i2c read error, ret %d\n", __func__,__LINE__,ret);
				//return ret;
			}
		}
		tp_log_info("buf_recv 0x%x, 0x%x\n", buf_recv[newMaxSize*i], buf_recv[newMaxSize*i + 1]);
	}
	for(i = 0; i < test_32; i++) {
		if (j == i2c.y_ch * i2c.x_ch) {
			break;
		}
		for(index = 2; index < newMaxSize; index += 2) {
			if (((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]) < opentestthrehold) {
				open_test_result = -1;
			}
			sprintf(buf + len, "%4d, ", (buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]);
			j++;
			len += 6;
			if(j % i2c.x_ch == 0) {
				sprintf(buf + len, "\n");
				len += 1;
			}
			if (j == i2c.y_ch * i2c.x_ch) {
				break;
			}
		}
	}
	sprintf(buf + len, "\n");
	len += 1;
	tp_log_info("ilitek_open_test len = %d\n", len);
	ret = ilitek_into_normalmode();
	ilitek_i2c_irq_enable();
	if (buf_recv) {
		kfree(buf_recv);
		buf_recv = NULL;
	}
	return len;
}
static DEVICE_ATTR(ilitek_open_test, S_IRUGO, ilitek_open_test_show, NULL);

static ssize_t ilitek_short_test_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0, newMaxSize = 32, i = 0, j = 0, index = 0, len = 0;
	uint8_t cmd[4] = {0};
	uint8_t * buf_recv = kzalloc(sizeof(uint8_t) * i2c.x_ch * 2 + 32, GFP_KERNEL);
	int * x1_data = kzalloc(sizeof(int) * i2c.x_ch + 32, GFP_KERNEL);
	int * x2_data = kzalloc(sizeof(int) * i2c.x_ch + 32, GFP_KERNEL);
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	struct i2c_msg msgs_ret[] = {
		{.addr = i2c.client->addr, .flags = I2C_M_RD, .len = 3, .buf = buf_recv,}
	};
	int test_32 = (i2c.x_ch * 2) / (newMaxSize - 2);
	if ((i2c.x_ch * 2) % (newMaxSize - 2) != 0) {
		test_32 += 1;
	}
	short_test_result = 0;
	ilitek_i2c_irq_disable();
	ret = ilitek_ready_into_test();
	ret = ilitek_into_testmode();
	#if 1
	cmd[0] = 0xF1;
	cmd[1] = 0x04;
	cmd[2] = 0x00;
	msgs_cmd[0].len = 3;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		//return ret;
	}
	mdelay(350);
	cmd[0] = 0xF6;
	cmd[1] = 0xF2;
	msgs_cmd[0].len = 2;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		//return ret;
	}
	cmd[0] = 0xF2;
	msgs_cmd[0].len = 1;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		//return ret;
	}
	for(i = 0; i < test_32; i++){
		if ((i2c.x_ch * 2)%(newMaxSize - 2) != 0 && i == test_32 - 1) {
			msgs_ret[0].len = (i2c.x_ch * 2)%(newMaxSize - 2) + 2;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				tp_log_err("%s:%d, i2c read error, ret %d\n", __func__,__LINE__,ret);
				//return ret;
			}
		}
		else {
			msgs_ret[0].len = newMaxSize;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				tp_log_err("%s:%d, i2c read error, ret %d\n", __func__,__LINE__,ret);
				//return ret;
			}
		}
		tp_log_info("buf_recv 0x%x, 0x%x\n", buf_recv[newMaxSize*i], buf_recv[newMaxSize*i + 1]);
	}
	j = 0;
	len = 0;
	for(i = 0; i < test_32; i++) {
		if (j == i2c.x_ch * 2) {
			break;
		}
		for(index = 2; index < newMaxSize; index++) {
			if (j < i2c.x_ch) {
				x1_data[j] = buf_recv[i * newMaxSize + index];
			}
			else {
				x2_data[j - i2c.x_ch] = buf_recv[i * newMaxSize + index];
			}
			sprintf(buf + len, "%4d, ", buf_recv[i * newMaxSize + index]);
			j++;
			len += 6;
			if(j % i2c.x_ch == 0) {
				sprintf(buf + len, "\n");
				len += 1;
			}
			if (j == i2c.x_ch * 2) {
				break;
			}
		}
	}
	for (i = 0; i < i2c.x_ch; i++) {
		if (abs(x1_data[i] - x2_data[i]) > microshorttestwindow) {
			short_test_result = -1;
		}
	}
	sprintf(buf + len, "\n");
	len += 1;
	#endif
	tp_log_info("ilitek_short_test len = %d\n", len);
	ret = ilitek_into_normalmode();
	ilitek_i2c_irq_enable();
	if (buf_recv) {
		kfree(buf_recv);
		buf_recv = NULL;
	}
	if (x1_data) {
		kfree(x1_data);
		x1_data = NULL;
	}
	if (x2_data) {
		kfree(x2_data);
		x2_data = NULL;
	}

	return len;
}

static DEVICE_ATTR(ilitek_short_test, S_IRUGO, ilitek_short_test_show, NULL);
static ssize_t ilitek_allnode_test_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0, newMaxSize = 32, i = 0, j = 0, index = 0, len = 0;
	uint8_t cmd[4] = {0};
	uint8_t * buf_recv = kzalloc(sizeof(uint8_t) * i2c.y_ch * i2c.x_ch * 2 + 32, GFP_KERNEL);
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	struct i2c_msg msgs_ret[] = {
		{.addr = i2c.client->addr, .flags = I2C_M_RD, .len = 3, .buf = buf_recv,}
	};
	int test_32 = (i2c.y_ch * i2c.x_ch * 2) / (newMaxSize - 2);
	if ((i2c.y_ch * i2c.x_ch * 2) % (newMaxSize - 2) != 0) {
		test_32 += 1;
	}
	if (!buf_recv) {
		//tp_log_err("ilitek_all_test buf_recv kzalloc error\n");
	}
	allnode_test_result = 0;
	allnode_self_test_result = 0;

	ilitek_i2c_irq_disable();
	ret = ilitek_ready_into_test();
	ret = ilitek_into_testmode();
	cmd[0] = 0xF1;
	cmd[1] = 0x08;
	cmd[2] = 0x00;
	msgs_cmd[0].len = 3;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		//return ret;
	}
	mdelay(350);
	cmd[0] = 0xF6;
	cmd[1] = 0xF2;
	msgs_cmd[0].len = 2;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		//return ret;
	}
	cmd[0] = 0xF2;
	msgs_cmd[0].len = 1;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		//return ret;
	}
	for(i = 0; i < test_32; i++){
		if ((i2c.y_ch * i2c.x_ch * 2)%(newMaxSize - 2) != 0 && i == test_32 - 1) {
			msgs_ret[0].len = (i2c.y_ch * i2c.x_ch * 2)%(newMaxSize - 2) + 2;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				tp_log_err("%s:%d, i2c read error, ret %d\n", __func__,__LINE__,ret);
				//return ret;
			}
		}
		else {
			msgs_ret[0].len = newMaxSize;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				tp_log_err("%s:%d, i2c read error, ret %d\n", __func__,__LINE__,ret);
				//return ret;
			}
		}
		tp_log_info("buf_recv 0x%x, 0x%x\n", buf_recv[newMaxSize*i], buf_recv[newMaxSize*i + 1]);
	}
	for(i = 0; i < test_32; i++) {
		if (j == i2c.y_ch * i2c.x_ch) {
			break;
		}
		for(index = 2; index < newMaxSize; index += 2) {
			if (((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]) < allnodetestymin || 
				((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]) > allnodetestymax) {
				allnode_test_result = -1;
			}
			sprintf(buf + len, "%4d, ", (buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]);
			j++;
			len += 6;
			if(j % i2c.x_ch == 0) {
				sprintf(buf + len, "\n");
				len += 1;
			}
			if (j == i2c.y_ch * i2c.x_ch) {
				break;
			}
		}
	}
	sprintf(buf + len, "\n");
	len += 1;
	test_32 = (i2c.x_ch * 2) / (newMaxSize - 2);
	if ((i2c.x_ch * 2) % (newMaxSize - 2) != 0) {
		test_32 += 1;
	}
	cmd[0] = 0xF1;
	cmd[1] = 0x0B;
	cmd[2] = 0x00;
	msgs_cmd[0].len = 3;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		//return ret;
	}
	mdelay(350);
	cmd[0] = 0xF6;
	cmd[1] = 0xF2;
	msgs_cmd[0].len = 2;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		//return ret;
	}
	cmd[0] = 0xF2;
	msgs_cmd[0].len = 1;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_err("%s:%d err, ret %d\n", __func__,__LINE__,ret);
		//return ret;
	}
	for(i = 0; i < test_32; i++){
		if ((i2c.x_ch * 2)%(newMaxSize - 2) != 0 && i == test_32 - 1) {
			msgs_ret[0].len = (i2c.x_ch * 2)%(newMaxSize - 2) + 2;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				tp_log_err("%s:%d, i2c read error, ret %d\n", __func__,__LINE__,ret);
				//return ret;
			}
		}
		else {
			msgs_ret[0].len = newMaxSize;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				tp_log_err("%s:%d, i2c read error, ret %d\n", __func__,__LINE__,ret);
				//return ret;
			}
		}
		tp_log_info("buf_recv 0x%x, 0x%x\n", buf_recv[newMaxSize*i], buf_recv[newMaxSize*i + 1]);
	}
	for(i = 0; i < test_32; i++) {
		if (j == i2c.x_ch) {
			break;
		}
		for(index = 2; index < newMaxSize; index += 2) {
			if (((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]) < allnodetestymin_self|| 
				((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]) > allnodetestymax_self) {
				allnode_self_test_result = -1;
			}
			sprintf(buf + len, "%4d, ", (buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]);
			j++;
			len += 6;
			if(j % i2c.x_ch == 0) {
				sprintf(buf + len, "\n");
				len += 1;
			}
			if (j == i2c.x_ch) {
				break;
			}
		}
	}
	sprintf(buf + len, "\n");
	len += 1;
	tp_log_info("ilitek_allnode_test len = %d\n", len);
	ret = ilitek_into_normalmode();
	ilitek_i2c_irq_enable();
	if (buf_recv) {
		kfree(buf_recv);
		buf_recv = NULL;
	}
	return len;
}

static DEVICE_ATTR(ilitek_allnode_test, S_IRUGO, ilitek_allnode_test_show, NULL);

static struct attribute *sysfs_attrs_ctrl[] = {
	&dev_attr_ilitek_open_test.attr,
	&dev_attr_ilitek_short_test.attr,
	&dev_attr_ilitek_allnode_test.attr,
	NULL
};
static struct attribute_group ilitek_attribute_group[] = {
	{.attrs = sysfs_attrs_ctrl },
};

#if 1
static ssize_t ilitek_open_test_store(struct kobject *dev,
		struct kobj_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	ret = kstrtoint(buf, 10, &opentestthrehold);
	tp_log_info("ilitek_open_test_store opentestthrehold = %d\n", opentestthrehold);
	if (ret) {
		tp_log_err("ilitek_open_test_store kstrtoint error\n");
		return ret;
	}
	return size;
}

static struct kobj_attribute ilitek_open_test_set = {
	.attr = {.name = "ilitek_open_test_set", .mode = 0664},
	.show = NULL,
	.store = ilitek_open_test_store,
};

static ssize_t ilitek_short_test_store(struct kobject *dev,
		struct kobj_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	ret = kstrtoint(buf, 10, &microshorttestwindow);
	tp_log_info("ilitek_short_test_store microshorttestwindow = %d\n", microshorttestwindow);
	if (ret) {
		tp_log_err("ilitek_short_test_store kstrtoint error\n");
		return ret;
	}
	return size;
}

static struct kobj_attribute ilitek_short_test_set = {
	.attr = {.name = "ilitek_short_test_set", .mode = 0664},
	.show = NULL,
	.store = ilitek_short_test_store,
};

static ssize_t ilitek_allnode_test_store(struct kobject *dev,
		struct kobj_attribute *attr, const char *buf, size_t size)
{	
	if (sscanf(buf, "%d:%d:%d:%d\n", &allnodetestymax, &allnodetestymin, &allnodetestymax_self, &allnodetestymin_self) != 4){
		tp_log_err("ilitek cmd format error\n");
		return -EINVAL;
	}
	tp_log_info("ilitek_allnode_test_store allnodetestymax = %d\n", allnodetestymax);
	tp_log_info("ilitek_allnode_test_store allnodetestymin = %d\n", allnodetestymin);
	tp_log_info("ilitek_allnode_test_store allnodetestymax_self = %d\n", allnodetestymax_self);
	tp_log_info("ilitek_allnode_test_store allnodetestymin_self = %d\n", allnodetestymin_self);
	return size;
}
static struct kobj_attribute ilitek_allnode_test_set = {
	.attr = {.name = "ilitek_allnode_test_set", .mode = 0664},
	.show = NULL,
	.store = ilitek_allnode_test_store,
};
#endif

 int ilitek_rawdata_proc_show(struct seq_file *m, void *v)
{
	int i = 0;
	int j = 0;
	int ret = -1;
	int * short_data1 = kzalloc(sizeof(int) * i2c.x_ch + 32, GFP_KERNEL);
	int * short_data2 = kzalloc(sizeof(int) * i2c.x_ch + 32, GFP_KERNEL);
	int * open_data = kzalloc(sizeof(int) * i2c.y_ch * i2c.x_ch + 32, GFP_KERNEL);
	int * allnode_data = kzalloc(sizeof(int) * i2c.y_ch * i2c.x_allnode_ch + 32, GFP_KERNEL);
	int * allnode_self_data = kzalloc(sizeof(int)  * i2c.x_ch + 32, GFP_KERNEL);

    if(NULL == short_data1||NULL == short_data2||NULL == open_data
		||NULL == allnode_data||NULL == allnode_self_data){
		tp_log_err("%s, invalid parameter NULL\n", __func__);
		return -ENOMEM;
    }
	ilitek_i2c_irq_disable();
	ret = ilitek_ready_into_test();
	if (ret){
		tp_log_err("%s, i2c transfer error, ret = %d\n", __func__, ret);
		seq_printf(m, "%s", "0F-");
		return ret;
	}

	ret = ilitek_into_testmode();
	mdelay(30);
	ret = ilitek_short_test(short_data1, short_data2);
	if (ret){
		tp_log_err("%s, short test failed, ret = %d\n", __func__, ret);
		seq_printf(m, "%s", "0P-1F-");
	}
	else{
		seq_printf(m, "%s", "0P-1P-");
	}
	ret = ilitek_open_test(open_data);
	if (ret){
		tp_log_err("%s, open test failed, ret = %d\n", __func__, ret);
		seq_printf(m, "%s", "2F-");
	}
	else{
		seq_printf(m, "%s", "2P-");
	}

	ret = ilitek_allnode_test(allnode_data);
	if (ret){
		tp_log_err("%s, allnode test failed, ret = %d\n", __func__, ret);
		seq_printf(m, "%s", "3F-");
	}
	else{
		seq_printf(m, "%s", "3P-");
	}

	ret = ilitek_allnode_self_test(allnode_self_data);
	if (ret){
		tp_log_err("%s, allnode_self test failed, ret = %d\n", __func__, ret);
		seq_printf(m, "%s", "4F-");
	}
	else{
		seq_printf(m, "%s", "4P-");
	}

	ret = ilitek_into_normalmode();
	ilitek_reset(i2c.reset_gpio);
	ilitek_i2c_irq_enable();

	seq_printf(m, "%s", "\n");
	seq_printf(m, "%s", "*****i2c_connect***open***short***allnode*****\n");
	seq_printf(m, "%s", "***rawdata***\n");
	for (j = 0; j < i2c.y_ch * i2c.x_allnode_ch; j++) {
		seq_printf(m, "%4d,", allnode_data[j]);
		i++;
		if((i) % i2c.x_allnode_ch == 0) {
			seq_printf(m, "%s", "\n");
		}
	}
	seq_printf(m, "%s", "\n");

	seq_printf(m, "%s", "***rawdata_self***\n");
	for (j = 0; j < i2c.x_ch; j++) {
		seq_printf(m, "%4d,", allnode_self_data[j]);
	}
	seq_printf(m, "%s", "\n");

	seq_printf(m, "%s", "***short***\n");
	for (j = 0; j < i2c.x_ch; j++) {
		seq_printf(m, "%3d,", short_data1[j]);
	}
	seq_printf(m, "%s", "\n");
	for (j = 0; j < i2c.x_ch; j++) {
		seq_printf(m, "%3d,", short_data2[j]);
	}
	seq_printf(m, "%s", "\n");
	seq_printf(m, "%s", "\n");
	/*release momery*/
	if (short_data1) {
		kfree(short_data1);
		short_data1 = NULL;
	}
	if (short_data2) {
		kfree(short_data2);
		short_data2 = NULL;
	}
	if (open_data) {
		kfree(open_data);
		open_data = NULL;
	}
	if (allnode_data) {
		kfree(allnode_data);
		allnode_data = NULL;
	}
	if (allnode_self_data) {
		kfree(allnode_self_data);
		allnode_self_data = NULL;
	}
	return 0;
}

 int ilitek_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ilitek_rawdata_proc_show, NULL);
}

 const struct file_operations ilitek_proc_fops = {
	.open = ilitek_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
static ssize_t hw_touch_test_type_show(struct kobject *dev,
		struct kobj_attribute *attr, char *buf)
{

	return snprintf(buf, PAGE_SIZE - 1,
		"%s:%s\n",
		NORMALIZE_TP_CAPACITANCE_TEST,
		TP_JUDGE_SYNAPTICS_RESULT);
}

static struct kobj_attribute hw_touch_test_type = {
	.attr = {.name = "tp_capacitance_test_type", .mode = 0444},
	.show = hw_touch_test_type_show,
	.store = NULL,
};

void ilitek_touch_node_init(void)
{
	int ret ;
	struct proc_dir_entry *proc_entry;
	
	android_touch_kobj = kobject_create_and_add("touchscreen", NULL) ;
	if (android_touch_kobj == NULL) {
		tp_log_err(KERN_ERR "[ilitek]%s: kobject_create_and_add failed\n", __func__);
		return;
	}
	ret = sysfs_create_group(android_touch_kobj, ilitek_attribute_group);
	if (ret < 0) {
		tp_log_err("%s: sysfs_create_group failed\n", __func__);
	}
	tp_log_info("ilitek_open_test_set sysfs_create_file\n");
	ret = sysfs_create_file(android_touch_kobj, &ilitek_open_test_set.attr);
	if (ret < 0) {
		tp_log_err("%s: sysfs_create_group failed\n", __func__);
	}
	tp_log_info("ilitek_short_test_set sysfs_create_file\n");
	ret = sysfs_create_file(android_touch_kobj, &ilitek_short_test_set.attr);
	if (ret < 0) {
		tp_log_err("%s: sysfs_create_group failed\n", __func__);
	}
	tp_log_info("ilitek_allnode_test_set sysfs_create_file\n");
	ret = sysfs_create_file(android_touch_kobj, &ilitek_allnode_test_set.attr);
	if (ret < 0) {
		tp_log_err("%s: sysfs_create_group failed\n", __func__);
	}

	ret = sysfs_create_file(android_touch_kobj, &hw_touch_test_type.attr);
	if (ret) {
		//kobject_put(properties_kobj);
		tp_log_err("%s: ilitek_mmi_test create file error\n", __func__);
		//return -ENODEV;
	}
	proc_entry = proc_mkdir("touchscreen", NULL);
	if (!proc_entry) {
		tp_log_err("%s:%d Error, failed to creat procfs.\n",__func__, __LINE__);
		//return -EINVAL;
	}

	if (!proc_create("tp_capacitance_data", 0, proc_entry, &ilitek_proc_fops)) {
		tp_log_err("%s:%d Error, failed to creat procfs.\n",__func__, __LINE__);
		remove_proc_entry("tp_capacitance_data", proc_entry);
		//return -EINVAL;
	}
	return;
}

void ilitek_touch_node_deinit(void)
{
	sysfs_remove_group(android_touch_kobj, ilitek_attribute_group);
	kobject_del(android_touch_kobj);
}

#endif
#endif
