/* Copyright (c) 2014 - 2016, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) "%s:%d " fmt, __func__, __LINE__

#include <linux/module.h>
#include <linux/of_gpio.h>
#include "msm_sd.h"
#include "msm_ois.h"
#include "msm_cci.h"

DEFINE_MSM_MUTEX(msm_ois_mutex);
/*#define MSM_OIS_DEBUG*/
#undef CDBG
#ifdef MSM_OIS_DEBUG
#define CDBG(fmt, args...) pr_err(fmt, ##args)
#else
#define CDBG(fmt, args...) pr_debug(fmt, ##args)
#endif

#define MAX_POLL_COUNT 100

static struct v4l2_file_operations msm_ois_v4l2_subdev_fops;
static int32_t msm_ois_power_up(struct msm_ois_ctrl_t *o_ctrl);
static int32_t msm_ois_power_down(struct msm_ois_ctrl_t *o_ctrl);

static struct i2c_driver msm_ois_i2c_driver;
enum reg_size_type{
	REG_DATA_SIZE_BYTE = 1,
	REG_DATA_SIZE_WORD = 2,
	REG_DATA_SIZE_DWORD = 4,
};

/*
ois register operation: read,write,poll
*/
static int32_t msm_ois_reg_settings(struct msm_ois_ctrl_t *o_ctrl,
	uint16_t size, struct reg_settings_ois_t *settings, struct reg_settings_ois_t *user_settings)
{
	int32_t rc = -EFAULT;
	int32_t i = 0;
	struct msm_camera_i2c_seq_reg_array *reg_setting = NULL;
	CDBG("Enter\n");

	reg_setting = kzalloc(sizeof(struct msm_camera_i2c_seq_reg_array), GFP_KERNEL);
	if (!reg_setting){
		pr_err("reg_setting kzalloc fail \n");
		return -ENOMEM;
	}

	for (i = 0; i < size; i++) {
		switch (settings[i].i2c_operation) {
		case MSM_OIS_READ: {
			memset(reg_setting, 0, sizeof(struct msm_camera_i2c_seq_reg_array));
			reg_setting->reg_addr = settings[i].reg_addr;

			switch (settings[i].data_type) {
			case MSM_CAMERA_I2C_BYTE_DATA:
				reg_setting->reg_data_size = REG_DATA_SIZE_BYTE;
				rc = o_ctrl->i2c_client.i2c_func_tbl->i2c_read_seq(
					&o_ctrl->i2c_client,
					reg_setting->reg_addr,
					reg_setting->reg_data,
					reg_setting->reg_data_size);
				settings[i].reg_data = reg_setting->reg_data[0];
				rc  = copy_to_user((void __user *)&user_settings[i].reg_data, &settings[i].reg_data, sizeof(settings[i].reg_data));
				if(rc)
				{
				    pr_err("cannot copy error rc=%d \n", rc);
				}
				break;
			case MSM_CAMERA_I2C_WORD_DATA:
				reg_setting->reg_data_size = REG_DATA_SIZE_WORD;
				rc = o_ctrl->i2c_client.i2c_func_tbl->i2c_read_seq(
					&o_ctrl->i2c_client,
					reg_setting->reg_addr,
					reg_setting->reg_data,
					reg_setting->reg_data_size);
				settings[i].reg_data = reg_setting->reg_data[0] <<8|reg_setting->reg_data[1];
				rc  = copy_to_user((void __user *)&user_settings[i].reg_data, &settings[i].reg_data, sizeof(settings[i].reg_data));
				if(rc)
				{
				    pr_err("cannot copy error rc=%d \n", rc);
				}
				break;
			case MSM_CAMERA_I2C_DWORD_DATA:
				reg_setting->reg_data_size = REG_DATA_SIZE_DWORD;
				rc = o_ctrl->i2c_client.i2c_func_tbl->i2c_read_seq(
					&o_ctrl->i2c_client,
					reg_setting->reg_addr,
					reg_setting->reg_data,
					reg_setting->reg_data_size);
				settings[i].reg_data = (reg_setting->reg_data[0] << 24)|(reg_setting->reg_data[1] << 16)
					|(reg_setting->reg_data[2] << 8) |(reg_setting->reg_data[3]);
				rc  = copy_to_user((void __user *)&user_settings[i].reg_data, &settings[i].reg_data, sizeof(settings[i].reg_data));
				if(rc)
				{
				    pr_err("cannot copy error rc=%d \n", rc);
				}
				break;
			case MSM_CAMERA_I2C_SEQ:
				reg_setting->reg_data_size = settings[i].seq_size;
				rc = o_ctrl->i2c_client.i2c_func_tbl->i2c_read_seq(
					&o_ctrl->i2c_client,
					reg_setting->reg_addr,
					reg_setting->reg_data,
					reg_setting->reg_data_size);
				rc  = copy_to_user((void __user *)&user_settings[i].reg_seq, &reg_setting->reg_data, sizeof(char)*settings[i].seq_size);
				if(rc)
				{
				    pr_err("cannot copy error rc=%d \n", rc);
				}
				break;
			default:
				pr_err("Unsupport data type: %d\n",
					settings[i].data_type);
				break;
			}
			if (settings[i].delay > 20)
				msleep(settings[i].delay);
			else if (0 != settings[i].delay)
				usleep_range(settings[i].delay * 1000,
					(settings[i].delay * 1000) + 1000);
		}
			break;

		case MSM_OIS_WRITE: {
			switch (settings[i].data_type) {
			case MSM_CAMERA_I2C_BYTE_DATA:
			case MSM_CAMERA_I2C_WORD_DATA:
				rc = o_ctrl->i2c_client.i2c_func_tbl->i2c_write(
					&o_ctrl->i2c_client,
					settings[i].reg_addr,
					settings[i].reg_data,
					settings[i].data_type);
				break;
			case MSM_CAMERA_I2C_DWORD_DATA:
				memset(reg_setting, 0, sizeof(struct msm_camera_i2c_seq_reg_array));
				reg_setting->reg_addr = settings[i].reg_addr;
				reg_setting->reg_data[0] = (uint8_t)
					((settings[i].reg_data &
					0xFF000000) >> 24);
				reg_setting->reg_data[1] = (uint8_t)
					((settings[i].reg_data &
					0x00FF0000) >> 16);
				reg_setting->reg_data[2] = (uint8_t)
					((settings[i].reg_data &
					0x0000FF00) >> 8);
				reg_setting->reg_data[3] = (uint8_t)
					(settings[i].reg_data & 0x000000FF);
				reg_setting->reg_data_size = REG_DATA_SIZE_DWORD;
				rc = o_ctrl->i2c_client.i2c_func_tbl->
					i2c_write_seq(&o_ctrl->i2c_client,
					reg_setting->reg_addr,
					reg_setting->reg_data,
					reg_setting->reg_data_size);
				break;
			case MSM_CAMERA_I2C_SEQ:
				reg_setting->reg_addr = settings[i].reg_addr;
				memcpy(reg_setting->reg_data, settings[i].reg_seq, settings[i].seq_size);
				reg_setting->reg_data_size = settings[i].seq_size;
				rc = o_ctrl->i2c_client.i2c_func_tbl->
					i2c_write_seq(&o_ctrl->i2c_client,
					reg_setting->reg_addr,
					reg_setting->reg_data,
					reg_setting->reg_data_size);
				break;
			default:
				pr_err("Unsupport data type: %d\n",
					settings[i].data_type);
				break;
			}
			if (settings[i].delay > 20)
				msleep(settings[i].delay);
			else if (0 != settings[i].delay)
				usleep_range(settings[i].delay * 1000,
					(settings[i].delay * 1000) + 1000);
		}
			break;

		case MSM_OIS_POLL: {
			int32_t poll_count = 0;
			uint32_t value = 0;
			uint32_t user_count = 0;
			uint32_t user_sleep = 0;

			switch (settings[i].data_type) {
			case MSM_CAMERA_I2C_BYTE_DATA:
			case MSM_CAMERA_I2C_WORD_DATA:
				rc = o_ctrl->i2c_client.i2c_func_tbl
					->i2c_poll(&o_ctrl->i2c_client,
					settings[i].reg_addr,
					settings[i].reg_data,
					settings[i].data_type,
					settings[i].delay);
				break;
			case MSM_CAMERA_I2C_DWORD_DATA:
				user_sleep = 5;/*need delay 5ms to read seq*/
				user_count = settings[i].delay/user_sleep;/*retry times*/
				memset(reg_setting, 0, sizeof(struct msm_camera_i2c_seq_reg_array));
				do {
					reg_setting->reg_addr = settings[i].reg_addr;
					reg_setting->reg_data_size = REG_DATA_SIZE_DWORD;
					rc = o_ctrl->i2c_client.i2c_func_tbl->i2c_read_seq(
					&o_ctrl->i2c_client,
					reg_setting->reg_addr,
					reg_setting->reg_data,
					reg_setting->reg_data_size);
					value = (reg_setting->reg_data[0] << 24)|(reg_setting->reg_data[1] << 16)
					|(reg_setting->reg_data[2] << 8) |(reg_setting->reg_data[3]);
					CDBG("MSM_OIS_POLL value=0x%08x\n", value);
					CDBG("user_count=%d, MAX_POLL_COUNT=%d \n", user_count, MAX_POLL_COUNT);
					user_count = user_count > MAX_POLL_COUNT?MAX_POLL_COUNT:user_count;
					if (poll_count++ > user_count) {
						pr_err("MSM_OIS_POLL failed poll_count=%d", poll_count);
						break;
					}
					if(rc !=0 ){
						pr_err("MSM_OIS_POLL failed rc =%d", rc );
						break;
					}
					if(value != settings[i].reg_data){
						pr_err("value=%d,  settings[i].reg_data=%d", value,  settings[i].reg_data);
						usleep_range(user_sleep * 1000,(user_sleep * 1000) + 1000);
					}

				}while (value != settings[i].reg_data) ;

				if(value == settings[i].reg_data){
					pr_err("MSM_OIS_POLL success reg_addr=0x%04x, reg_data=0x%08x\n", reg_setting->reg_addr, value);
				}
				else {
					pr_err("MSM_OIS_POLL fail reg_addr=0x%04x, reg_value=0x%08x,but we hope is 0x%08x\n", reg_setting->reg_addr, value, settings[i].reg_data);
					rc = -1;/*poll fail need return -1*/
				}
				break;
			default:
				pr_err("Unsupport data type: %d\n",
					settings[i].data_type);
				break;
			}
		}
		}

		if (rc < 0){
			pr_err("failed \n");
			break;
		}
	}

	CDBG("Exit\n");
	if(reg_setting){
		kfree(reg_setting);
		reg_setting = NULL;
	}
	return rc;
}

static int32_t msm_ois_vreg_control(struct msm_ois_ctrl_t *o_ctrl,
							int config)
{
	int rc = 0, i, cnt;
	struct msm_ois_vreg *vreg_cfg;

	vreg_cfg = &o_ctrl->vreg_cfg;
	cnt = vreg_cfg->num_vreg;
	if (!cnt)
		return 0;

	if (cnt >= MSM_OIS_MAX_VREGS) {
		pr_err("failed cnt %d\n", cnt);
		return -EINVAL;
	}

	for (i = 0; i < cnt; i++) {
		rc = msm_camera_config_single_vreg(&(o_ctrl->pdev->dev),
			&vreg_cfg->cam_vreg[i],
			(struct regulator **)&vreg_cfg->data[i],
			config);
	}
	return rc;
}

static int32_t msm_ois_power_down(struct msm_ois_ctrl_t *o_ctrl)
{
	int32_t rc = 0;
	enum msm_sensor_power_seq_gpio_t gpio;

	CDBG("Enter\n");
	if (o_ctrl->ois_state != OIS_DISABLE_STATE) {

		rc = msm_ois_vreg_control(o_ctrl, 0);
		if (rc < 0) {
			pr_err("failed \n");
			return rc;
		}

		for (gpio = SENSOR_GPIO_AF_PWDM; gpio < SENSOR_GPIO_MAX;
			gpio++) {
			if (o_ctrl->gconf &&
				o_ctrl->gconf->gpio_num_info &&
				o_ctrl->gconf->
					gpio_num_info->valid[gpio] == 1) {
				gpio_set_value_cansleep(
					o_ctrl->gconf->gpio_num_info
						->gpio_num[gpio],
					GPIOF_OUT_INIT_LOW);

				if (o_ctrl->cam_pinctrl_status) {
					rc = pinctrl_select_state(
						o_ctrl->pinctrl_info.pinctrl,
						o_ctrl->pinctrl_info.
							gpio_state_suspend);
					if (rc < 0)
						pr_err("ERR:%s:%d cannot set pin to suspend state: %d",
							__func__, __LINE__, rc);
					devm_pinctrl_put(
						o_ctrl->pinctrl_info.pinctrl);
				}
				o_ctrl->cam_pinctrl_status = 0;
				rc = msm_camera_request_gpio_table(
					o_ctrl->gconf->cam_gpio_req_tbl,
					o_ctrl->gconf->cam_gpio_req_tbl_size,
					0);
				if (rc < 0)
					pr_err("ERR:%s:Failed in selecting state in ois power down: %d\n",
						__func__, rc);
			}
		}

		o_ctrl->i2c_tbl_index = 0;
		o_ctrl->ois_state = OIS_OPS_INACTIVE;
	}
	CDBG("Exit\n");
	return rc;
}

static int msm_ois_init(struct msm_ois_ctrl_t *o_ctrl)
{
	int rc = 0;
	CDBG("Enter\n");

	if (!o_ctrl) {
		pr_err("failed\n");
		return -EINVAL;
	}

	if (o_ctrl->ois_device_type == MSM_CAMERA_PLATFORM_DEVICE) {
		rc = o_ctrl->i2c_client.i2c_func_tbl->i2c_util(
			&o_ctrl->i2c_client, MSM_CCI_INIT);
		if (rc < 0)
			pr_err("cci_init failed\n");
	}
	o_ctrl->ois_state = OIS_OPS_ACTIVE;
	CDBG("Exit\n");
	return rc;
}

/* The write protect pin control for ois firmware update function */
static int32_t msm_ois_flash_wp_enable(struct msm_ois_ctrl_t *o_ctrl,
	struct msm_ois_set_info_t *set_info)
{
	int32_t rc = 0;
	int32_t wp_status = -1;
	uint16_t gpio;

	CDBG("Enter\n");
	if(!o_ctrl || !set_info){
		pr_err("NULL pointer error: o_ctrl=%p, set_info=%p", o_ctrl, set_info);
		return -1;
	}

	if(!o_ctrl->gpio_num_info.valid[SENSOR_GPIO_FLASH_WP]){
		pr_err("SENSOR_GPIO_FLASH_WP gpio is invalid!");
		return -1;
	}

	gpio=o_ctrl->gpio_num_info.gpio_num[SENSOR_GPIO_FLASH_WP];
	#if 1
	pr_err("wp_status = %d", wp_status);
	#else
	wp_status = gpio_get_value(gpio);
	pr_err("wp_status = %d before set", wp_status);

	if(set_info->ois_params.flash_wp_enable)
		gpio_set_value(gpio, 1);
	else
		gpio_set_value(gpio, 0);

	wp_status = gpio_get_value(gpio);
	pr_err("wp_status = %d after set", wp_status);
	#endif

	CDBG("Exit\n");
	return rc;
}

static int32_t msm_ois_control(struct msm_ois_ctrl_t *o_ctrl,
	struct msm_ois_set_info_t *set_info)
{
	struct reg_settings_ois_t *settings = NULL;
	int32_t rc = 0;
	struct msm_camera_cci_client *cci_client = NULL;
	CDBG("Enter\n");

	if (o_ctrl->ois_device_type == MSM_CAMERA_PLATFORM_DEVICE) {
		cci_client = o_ctrl->i2c_client.cci_client;
		cci_client->sid =
			set_info->ois_params.i2c_addr >> 1;
		cci_client->retries = 3;
		cci_client->id_map = 0;
		cci_client->cci_i2c_master = o_ctrl->cci_master;
		cci_client->i2c_freq_mode = set_info->ois_params.i2c_freq_mode;
	} else {
		o_ctrl->i2c_client.client->addr =
			set_info->ois_params.i2c_addr;
	}
	o_ctrl->i2c_client.addr_type = MSM_CAMERA_I2C_WORD_ADDR;


	if (set_info->ois_params.setting_size > 0 &&
		set_info->ois_params.setting_size
		< MAX_OIS_REG_SETTINGS) {
		settings = kmalloc(
			sizeof(struct reg_settings_ois_t) *
			(set_info->ois_params.setting_size),
			GFP_KERNEL);
		if (settings == NULL) {
			pr_err("Error allocating memory\n");
			return -EFAULT;
		}
		if (copy_from_user(settings,
			(void *)set_info->ois_params.settings,
			set_info->ois_params.setting_size *
			sizeof(struct reg_settings_ois_t))) {
			kfree(settings);
			pr_err("Error copying\n");
			return -EFAULT;
		}

		rc = msm_ois_reg_settings(o_ctrl,
			set_info->ois_params.setting_size,
			settings, set_info->ois_params.settings);
		kfree(settings);
		if (rc < 0) {
			pr_err("Error\n");
			return -EFAULT;
		}
	}

	CDBG("Exit\n");

	return rc;
}

static int32_t msm_ois_config(struct msm_ois_ctrl_t *o_ctrl,
	void __user *argp)
{
	struct msm_ois_cfg_data *cdata =
		(struct msm_ois_cfg_data *)argp;
	int32_t rc = 0;
	mutex_lock(o_ctrl->ois_mutex);
	CDBG("Enter\n");
	CDBG("type %d\n", cdata->cfgtype);
	switch (cdata->cfgtype) {
	case CFG_OIS_INIT:
		rc = msm_ois_init(o_ctrl);
		if (rc < 0)
			pr_err("msm_ois_init failed %d\n", rc);
		break;
	case CFG_OIS_POWERDOWN:
		rc = msm_ois_power_down(o_ctrl);
		if (rc < 0)
			pr_err("msm_ois_power_down failed %d\n", rc);
		break;
	case CFG_OIS_POWERUP:
		rc = msm_ois_power_up(o_ctrl);
		if (rc < 0)
			pr_err("Failed ois power up%d\n", rc);
		break;
	case CFG_OIS_CONTROL:
		rc = msm_ois_control(o_ctrl, &cdata->cfg.set_info);
		if (rc < 0)
			pr_err("Failed ois control%d\n", rc);
		break;
	case CFG_OIS_FLASH_WP_ENABLE:
		rc = msm_ois_flash_wp_enable(o_ctrl, &cdata->cfg.set_info);
		if (rc < 0)
			pr_err("Failed CFG_OIS_FLASH_WP_ENABLE %d\n", rc);
		break;

	case CFG_OIS_I2C_WRITE_SEQ_TABLE: {
		struct msm_camera_i2c_seq_reg_setting conf_array;
		struct msm_camera_i2c_seq_reg_array *reg_setting = NULL;

#ifdef CONFIG_COMPAT
		if (is_compat_task()) {
			memcpy(&conf_array,
				(void *)cdata->cfg.settings,
				sizeof(struct msm_camera_i2c_seq_reg_setting));
		} else
#endif
		if (copy_from_user(&conf_array,
			(void *)cdata->cfg.settings,
			sizeof(struct msm_camera_i2c_seq_reg_setting))) {
			pr_err("failed\n");
			rc = -EFAULT;
			break;
		}

		if (!conf_array.size) {
			pr_err("failed\n");
			rc = -EFAULT;
			break;
		}
		reg_setting = kzalloc(conf_array.size *
			(sizeof(struct msm_camera_i2c_seq_reg_array)),
			GFP_KERNEL);
		if (!reg_setting) {
			pr_err("failed\n");
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(reg_setting, (void *)conf_array.reg_setting,
			conf_array.size *
			sizeof(struct msm_camera_i2c_seq_reg_array))) {
			pr_err("failed\n");
			kfree(reg_setting);
			rc = -EFAULT;
			break;
		}

		conf_array.reg_setting = reg_setting;
		rc = o_ctrl->i2c_client.i2c_func_tbl->
			i2c_write_seq_table(&o_ctrl->i2c_client,
			&conf_array);
		kfree(reg_setting);
		break;
	}
	default:
		break;
	}
	mutex_unlock(o_ctrl->ois_mutex);
	CDBG("Exit\n");
	return rc;
}

static int32_t msm_ois_get_subdev_id(struct msm_ois_ctrl_t *o_ctrl,
	void *arg)
{
	uint32_t *subdev_id = (uint32_t *)arg;
	CDBG("Enter\n");
	if (!subdev_id) {
		pr_err("failed\n");
		return -EINVAL;
	}
	if (o_ctrl->ois_device_type == MSM_CAMERA_PLATFORM_DEVICE)
		*subdev_id = o_ctrl->pdev->id;
	else
		*subdev_id = o_ctrl->subdev_id;

	CDBG("subdev_id %d\n", *subdev_id);
	CDBG("Exit\n");
	return 0;
}

static struct msm_camera_i2c_fn_t msm_sensor_cci_func_tbl = {
	.i2c_read = msm_camera_cci_i2c_read,
	.i2c_read_seq = msm_camera_cci_i2c_read_seq,
	.i2c_write = msm_camera_cci_i2c_write,
	.i2c_write_table = msm_camera_cci_i2c_write_table,
	.i2c_write_seq = msm_camera_cci_i2c_write_seq,
	.i2c_write_seq_table = msm_camera_cci_i2c_write_seq_table,
	.i2c_write_table_w_microdelay =
		msm_camera_cci_i2c_write_table_w_microdelay,
	.i2c_util = msm_sensor_cci_i2c_util,
	.i2c_poll =  msm_camera_cci_i2c_poll,
};

static struct msm_camera_i2c_fn_t msm_sensor_qup_func_tbl = {
	.i2c_read = msm_camera_qup_i2c_read,
	.i2c_read_seq = msm_camera_qup_i2c_read_seq,
	.i2c_write = msm_camera_qup_i2c_write,
	.i2c_write_table = msm_camera_qup_i2c_write_table,
	.i2c_write_seq = msm_camera_qup_i2c_write_seq,
	.i2c_write_seq_table = msm_camera_qup_i2c_write_seq_table,
	.i2c_write_table_w_microdelay =
		msm_camera_qup_i2c_write_table_w_microdelay,
	.i2c_poll = msm_camera_qup_i2c_poll,
};

static int msm_ois_close(struct v4l2_subdev *sd,
	struct v4l2_subdev_fh *fh) {
	int rc = 0;
	struct msm_ois_ctrl_t *o_ctrl =  v4l2_get_subdevdata(sd);
	CDBG("Enter\n");
	if (!o_ctrl) {
		pr_err("failed\n");
		return -EINVAL;
	}
	mutex_lock(o_ctrl->ois_mutex);
	if (o_ctrl->ois_device_type == MSM_CAMERA_PLATFORM_DEVICE &&
		o_ctrl->ois_state != OIS_DISABLE_STATE) {
		rc = o_ctrl->i2c_client.i2c_func_tbl->i2c_util(
			&o_ctrl->i2c_client, MSM_CCI_RELEASE);
		if (rc < 0)
			pr_err("cci_init failed\n");
	}
	o_ctrl->ois_state = OIS_DISABLE_STATE;
	mutex_unlock(o_ctrl->ois_mutex);
	CDBG("Exit\n");
	return rc;
}

static const struct v4l2_subdev_internal_ops msm_ois_internal_ops = {
	.close = msm_ois_close,
};

static long msm_ois_subdev_ioctl(struct v4l2_subdev *sd,
			unsigned int cmd, void *arg)
{
	int rc;
	struct msm_ois_ctrl_t *o_ctrl = v4l2_get_subdevdata(sd);
	void __user *argp = (void __user *)arg;
	CDBG("Enter\n");
	CDBG("%s:%d o_ctrl %p argp %p\n", __func__, __LINE__, o_ctrl, argp);
	switch (cmd) {
	case VIDIOC_MSM_SENSOR_GET_SUBDEV_ID:
		return msm_ois_get_subdev_id(o_ctrl, argp);
	case VIDIOC_MSM_OIS_CFG:
		return msm_ois_config(o_ctrl, argp);
	case MSM_SD_SHUTDOWN:
		if (!o_ctrl->i2c_client.i2c_func_tbl) {
			pr_err("o_ctrl->i2c_client.i2c_func_tbl NULL\n");
			return -EINVAL;
		}
		rc = msm_ois_power_down(o_ctrl);
		if (rc < 0) {
			pr_err("%s:%d OIS Power down failed\n",
				__func__, __LINE__);
		}
		return msm_ois_close(sd, NULL);
	default:
		pr_err("%s: error invalid cmd = %d, %ld, %ld, %ld \n", __func__, cmd, VIDIOC_MSM_SENSOR_GET_SUBDEV_ID, VIDIOC_MSM_OIS_CFG, MSM_SD_SHUTDOWN);
		return -ENOIOCTLCMD;
	}
}

static int32_t msm_ois_power_up(struct msm_ois_ctrl_t *o_ctrl)
{
	int rc = 0;
	enum msm_sensor_power_seq_gpio_t gpio;

	CDBG("%s called\n", __func__);

	rc = msm_ois_vreg_control(o_ctrl, 1);
	if (rc < 0) {
		pr_err("%s failed %d\n", __func__, __LINE__);
		return rc;
	}

	for (gpio = SENSOR_GPIO_AF_PWDM;
		gpio < SENSOR_GPIO_MAX; gpio++) {
		if (o_ctrl->gconf && o_ctrl->gconf->gpio_num_info &&
			o_ctrl->gconf->gpio_num_info->valid[gpio] == 1) {
			rc = msm_camera_request_gpio_table(
				o_ctrl->gconf->cam_gpio_req_tbl,
				o_ctrl->gconf->cam_gpio_req_tbl_size, 1);
			if (rc < 0) {
				pr_err("ERR:%s:Failed in selecting state for ois: %d\n",
					__func__, rc);
				return rc;
			}
			if (o_ctrl->cam_pinctrl_status) {
				rc = pinctrl_select_state(
					o_ctrl->pinctrl_info.pinctrl,
					o_ctrl->pinctrl_info.gpio_state_active);
				if (rc < 0)
					pr_err("ERR:%s:%d cannot set pin to active state: %d",
						__func__, __LINE__, rc);
			}

			gpio_set_value_cansleep(
				o_ctrl->gconf->gpio_num_info->gpio_num[gpio],
				1);
		}
	}

	o_ctrl->ois_state = OIS_ENABLE_STATE;
	CDBG("Exit\n");
	return rc;
}

static struct v4l2_subdev_core_ops msm_ois_subdev_core_ops = {
	.ioctl = msm_ois_subdev_ioctl,
};

static struct v4l2_subdev_ops msm_ois_subdev_ops = {
	.core = &msm_ois_subdev_core_ops,
};

static const struct i2c_device_id msm_ois_i2c_id[] = {
	{"qcom,ois", (kernel_ulong_t)NULL},
	{ }
};

static int32_t msm_ois_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	struct msm_ois_ctrl_t *ois_ctrl_t = NULL;
	CDBG("Enter\n");

	if (client == NULL) {
		pr_err("msm_ois_i2c_probe: client is null\n");
		return -EINVAL;
	}

	ois_ctrl_t = kzalloc(sizeof(struct msm_ois_ctrl_t),
		GFP_KERNEL);
	if (!ois_ctrl_t) {
		pr_err("%s:%d failed no memory\n", __func__, __LINE__);
		return -ENOMEM;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("i2c_check_functionality failed\n");
		rc = -EINVAL;
		goto probe_failure;
	}

	CDBG("client = 0x%p\n",  client);

	rc = of_property_read_u32(client->dev.of_node, "cell-index",
		&ois_ctrl_t->subdev_id);
	CDBG("cell-index %d, rc %d\n", ois_ctrl_t->subdev_id, rc);
	if (rc < 0) {
		pr_err("failed rc %d\n", rc);
		goto probe_failure;
	}

	ois_ctrl_t->i2c_driver = &msm_ois_i2c_driver;
	ois_ctrl_t->i2c_client.client = client;
	/* Set device type as I2C */
	ois_ctrl_t->ois_device_type = MSM_CAMERA_I2C_DEVICE;
	ois_ctrl_t->i2c_client.i2c_func_tbl = &msm_sensor_qup_func_tbl;
	ois_ctrl_t->ois_v4l2_subdev_ops = &msm_ois_subdev_ops;
	ois_ctrl_t->ois_mutex = &msm_ois_mutex;

	/* Assign name for sub device */
	snprintf(ois_ctrl_t->msm_sd.sd.name, sizeof(ois_ctrl_t->msm_sd.sd.name),
		"%s", ois_ctrl_t->i2c_driver->driver.name);

	/* Initialize sub device */
	v4l2_i2c_subdev_init(&ois_ctrl_t->msm_sd.sd,
		ois_ctrl_t->i2c_client.client,
		ois_ctrl_t->ois_v4l2_subdev_ops);
	v4l2_set_subdevdata(&ois_ctrl_t->msm_sd.sd, ois_ctrl_t);
	ois_ctrl_t->msm_sd.sd.internal_ops = &msm_ois_internal_ops;
	ois_ctrl_t->msm_sd.sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	media_entity_init(&ois_ctrl_t->msm_sd.sd.entity, 0, NULL, 0);
	ois_ctrl_t->msm_sd.sd.entity.type = MEDIA_ENT_T_V4L2_SUBDEV;
	ois_ctrl_t->msm_sd.sd.entity.group_id = MSM_CAMERA_SUBDEV_OIS;
	ois_ctrl_t->msm_sd.close_seq = MSM_SD_CLOSE_2ND_CATEGORY | 0x2;
	msm_sd_register(&ois_ctrl_t->msm_sd);
	ois_ctrl_t->ois_state = OIS_DISABLE_STATE;
	pr_info("msm_ois_i2c_probe: succeeded\n");
	CDBG("Exit\n");

probe_failure:
	kfree(ois_ctrl_t);
	return rc;
}

#ifdef CONFIG_COMPAT
static long msm_ois_subdev_do_ioctl(
	struct file *file, unsigned int cmd, void *arg)
{
	long rc = 0;
	struct video_device *vdev;
	struct v4l2_subdev *sd;
	struct msm_ois_cfg_data32 *u32;
	struct msm_ois_cfg_data ois_data;
	void *parg;
	struct msm_camera_i2c_seq_reg_setting settings;
	struct msm_camera_i2c_seq_reg_setting32 settings32;

	if (!file || !arg) {
		pr_err("%s:failed NULL parameter\n", __func__);
		return -EINVAL;
	}
	vdev = video_devdata(file);
	sd = vdev_to_v4l2_subdev(vdev);
	u32 = (struct msm_ois_cfg_data32 *)arg;
	parg = arg;

	ois_data.cfgtype = u32->cfgtype;

	switch (cmd) {
	case VIDIOC_MSM_OIS_CFG32:
		cmd = VIDIOC_MSM_OIS_CFG;

		switch (u32->cfgtype) {
		case CFG_OIS_CONTROL:
			ois_data.cfg.set_info.ois_params.setting_size =
				u32->cfg.set_info.ois_params.setting_size;
			ois_data.cfg.set_info.ois_params.i2c_addr =
				u32->cfg.set_info.ois_params.i2c_addr;
			ois_data.cfg.set_info.ois_params.i2c_freq_mode =
				u32->cfg.set_info.ois_params.i2c_freq_mode;
			ois_data.cfg.set_info.ois_params.i2c_addr_type =
				u32->cfg.set_info.ois_params.i2c_addr_type;
			ois_data.cfg.set_info.ois_params.i2c_data_type =
				u32->cfg.set_info.ois_params.i2c_data_type;
			ois_data.cfg.set_info.ois_params.settings =
				compat_ptr(u32->cfg.set_info.ois_params.
				settings);
			parg = &ois_data;
			break;
		case CFG_OIS_I2C_WRITE_SEQ_TABLE:
			if (copy_from_user(&settings32,
				(void *)compat_ptr(u32->cfg.settings),
				sizeof(
				struct msm_camera_i2c_seq_reg_setting32))) {
				pr_err("copy_from_user failed\n");
				return -EFAULT;
			}

			settings.addr_type = settings32.addr_type;
			settings.delay = settings32.delay;
			settings.size = settings32.size;
			settings.reg_setting =
				compat_ptr(settings32.reg_setting);

			ois_data.cfgtype = u32->cfgtype;
			ois_data.cfg.settings = &settings;
			parg = &ois_data;
			break;
		case CFG_OIS_FLASH_WP_ENABLE:
			pr_err("flash_wp_enable=%d\n", u32->cfg.set_info.ois_params.flash_wp_enable);
			ois_data.cfg.set_info.ois_params.flash_wp_enable =
				u32->cfg.set_info.ois_params.flash_wp_enable;
			parg = &ois_data;
			break;
		default:
			parg = &ois_data;
			break;
		}
	}
	rc = msm_ois_subdev_ioctl(sd, cmd, parg);

	return rc;
}

static long msm_ois_subdev_fops_ioctl(struct file *file, unsigned int cmd,
	unsigned long arg)
{
	return video_usercopy(file, cmd, arg, msm_ois_subdev_do_ioctl);
}
#endif

static int32_t msm_ois_platform_probe(struct platform_device *pdev)
{
	int32_t rc = 0;
	struct msm_camera_cci_client *cci_client = NULL;
	struct msm_ois_ctrl_t *msm_ois_t = NULL;
	struct msm_ois_vreg *vreg_cfg;
	uint16_t gpio_array_size = 0;
	uint16_t *gpio_array = NULL;
	int32_t val = 0;
	int32_t i = 0;
	struct device_node *of_node = NULL;

	CDBG("Enter\n");

	if (!pdev->dev.of_node) {
		pr_err("of_node NULL\n");
		return -EINVAL;
	}

	msm_ois_t = kzalloc(sizeof(struct msm_ois_ctrl_t),
		GFP_KERNEL);
	if (!msm_ois_t) {
		pr_err("%s:%d failed no memory\n", __func__, __LINE__);
		return -ENOMEM;
	}
	rc = of_property_read_u32((&pdev->dev)->of_node, "cell-index",
		&pdev->id);
	CDBG("cell-index %d, rc %d\n", pdev->id, rc);
	if (rc < 0) {
		kfree(msm_ois_t);
		pr_err("failed rc %d\n", rc);
		return rc;
	}

	rc = of_property_read_u32((&pdev->dev)->of_node, "qcom,cci-master",
		&msm_ois_t->cci_master);
	CDBG("qcom,cci-master %d, rc %d\n", msm_ois_t->cci_master, rc);
	if (rc < 0 || msm_ois_t->cci_master >= MASTER_MAX) {
		kfree(msm_ois_t);
		pr_err("failed rc %d\n", rc);
		return rc;
	}

	of_node = (&pdev->dev)->of_node;

	gpio_array_size = of_gpio_count(of_node);
	CDBG("gpio_array_size=%d\n", gpio_array_size);
	if (gpio_array_size) {
		gpio_array = kzalloc(sizeof(uint16_t) * gpio_array_size, GFP_KERNEL);
		if (!gpio_array) {
			pr_err("%s failed %d\n", __func__, __LINE__);
			kfree(msm_ois_t);
			return -ENOMEM;
		}
		for (i = 0; i < gpio_array_size; i++) {
			gpio_array[i] = of_get_gpio(of_node, i);
			CDBG("%s gpio_array[%d] = %d\n", __func__, i,
				gpio_array[i]);
		}
		rc = of_property_read_u32(of_node, "qcom,gpio-flash_wp", &val);
		if (rc < 0) {
			pr_err("%s:%dread qcom,gpio-flash_wp failed rc %d\n",
				__func__, __LINE__, rc);
		}
		else{
			msm_ois_t->gpio_num_info.gpio_num[SENSOR_GPIO_FLASH_WP] = gpio_array[val];
			msm_ois_t->gpio_num_info.valid[SENSOR_GPIO_FLASH_WP] = 1;

			CDBG("%s qcom,gpio-flash_wp %d index =%d \n", __func__,
				msm_ois_t->gpio_num_info.gpio_num[SENSOR_GPIO_FLASH_WP], val);
		}
		if(gpio_array)
			kfree(gpio_array);
	}

	if (of_find_property((&pdev->dev)->of_node,
			"qcom,cam-vreg-name", NULL)) {
		vreg_cfg = &msm_ois_t->vreg_cfg;
		rc = msm_camera_get_dt_vreg_data((&pdev->dev)->of_node,
			&vreg_cfg->cam_vreg, &vreg_cfg->num_vreg);
		if (rc < 0) {
			kfree(msm_ois_t);
			pr_err("failed rc %d\n", rc);
			return rc;
		}
	}

	rc = msm_sensor_driver_get_gpio_data(&(msm_ois_t->gconf),
		(&pdev->dev)->of_node);
	if (rc < 0) {
		pr_err("%s: No/Error OIS GPIO\n", __func__);
	} else {
		msm_ois_t->cam_pinctrl_status = 1;
		rc = msm_camera_pinctrl_init(
			&(msm_ois_t->pinctrl_info), &(pdev->dev));
		if (rc < 0) {
			pr_err("ERR:%s: Error in reading OIS pinctrl\n",
				__func__);
			rc = 0;
			msm_ois_t->cam_pinctrl_status = 0;
		}
		/*set ois fw wp_ctrl low*/
		if(msm_ois_t->gpio_num_info.valid[SENSOR_GPIO_FLASH_WP])
			gpio_set_value(msm_ois_t->gpio_num_info.gpio_num[SENSOR_GPIO_FLASH_WP], 0);
	}

	msm_ois_t->ois_v4l2_subdev_ops = &msm_ois_subdev_ops;
	msm_ois_t->ois_mutex = &msm_ois_mutex;

	/* Set platform device handle */
	msm_ois_t->pdev = pdev;
	/* Set device type as platform device */
	msm_ois_t->ois_device_type = MSM_CAMERA_PLATFORM_DEVICE;
	msm_ois_t->i2c_client.i2c_func_tbl = &msm_sensor_cci_func_tbl;
	msm_ois_t->i2c_client.cci_client = kzalloc(sizeof(
		struct msm_camera_cci_client), GFP_KERNEL);
	if (!msm_ois_t->i2c_client.cci_client) {
		kfree(msm_ois_t->vreg_cfg.cam_vreg);
		kfree(msm_ois_t);
		pr_err("failed no memory\n");
		return -ENOMEM;
	}

	cci_client = msm_ois_t->i2c_client.cci_client;
	cci_client->cci_subdev = msm_cci_get_subdev();
	cci_client->cci_i2c_master = msm_ois_t->cci_master;
	v4l2_subdev_init(&msm_ois_t->msm_sd.sd,
		msm_ois_t->ois_v4l2_subdev_ops);
	v4l2_set_subdevdata(&msm_ois_t->msm_sd.sd, msm_ois_t);
	msm_ois_t->msm_sd.sd.internal_ops = &msm_ois_internal_ops;
	msm_ois_t->msm_sd.sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	snprintf(msm_ois_t->msm_sd.sd.name,
		ARRAY_SIZE(msm_ois_t->msm_sd.sd.name), "msm_ois");
	media_entity_init(&msm_ois_t->msm_sd.sd.entity, 0, NULL, 0);
	msm_ois_t->msm_sd.sd.entity.type = MEDIA_ENT_T_V4L2_SUBDEV;
	msm_ois_t->msm_sd.sd.entity.group_id = MSM_CAMERA_SUBDEV_OIS;
	msm_ois_t->msm_sd.close_seq = MSM_SD_CLOSE_2ND_CATEGORY | 0x2;
	msm_sd_register(&msm_ois_t->msm_sd);
	msm_ois_t->ois_state = OIS_DISABLE_STATE;
	msm_cam_copy_v4l2_subdev_fops(&msm_ois_v4l2_subdev_fops);
#ifdef CONFIG_COMPAT
	msm_ois_v4l2_subdev_fops.compat_ioctl32 =
		msm_ois_subdev_fops_ioctl;
#endif
	msm_ois_t->msm_sd.sd.devnode->fops =
		&msm_ois_v4l2_subdev_fops;

	CDBG("Exit\n");
	return rc;
}

static const struct of_device_id msm_ois_i2c_dt_match[] = {
	{.compatible = "qcom,ois"},
	{}
};

MODULE_DEVICE_TABLE(of, msm_ois_i2c_dt_match);

static struct i2c_driver msm_ois_i2c_driver = {
	.id_table = msm_ois_i2c_id,
	.probe  = msm_ois_i2c_probe,
	.remove = __exit_p(msm_ois_i2c_remove),
	.driver = {
		.name = "qcom,ois",
		.owner = THIS_MODULE,
		.of_match_table = msm_ois_i2c_dt_match,
	},
};

static const struct of_device_id msm_ois_dt_match[] = {
	{.compatible = "qcom,ois", .data = NULL},
	{}
};

MODULE_DEVICE_TABLE(of, msm_ois_dt_match);

static struct platform_driver msm_ois_platform_driver = {
	.probe = msm_ois_platform_probe,
	.driver = {
		.name = "qcom,ois",
		.owner = THIS_MODULE,
		.of_match_table = msm_ois_dt_match,
	},
};

static int __init msm_ois_init_module(void)
{
	int32_t rc = 0;
	CDBG("Enter\n");
	rc = platform_driver_register(&msm_ois_platform_driver);
	if (!rc)
		return rc;
	CDBG("%s:%d rc %d\n", __func__, __LINE__, rc);
	return i2c_add_driver(&msm_ois_i2c_driver);
}

static void __exit msm_ois_exit_module(void)
{
	platform_driver_unregister(&msm_ois_platform_driver);
	i2c_del_driver(&msm_ois_i2c_driver);
	return;
}

module_init(msm_ois_init_module);
module_exit(msm_ois_exit_module);
MODULE_DESCRIPTION("MSM OIS");
MODULE_LICENSE("GPL v2");