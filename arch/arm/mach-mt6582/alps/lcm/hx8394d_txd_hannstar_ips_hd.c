/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2008
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/
#ifdef BUILD_LK
     #include <platform/upmu_common.h>
     #include <platform/mt_gpio.h>
     #include <platform/mt_i2c.h>
     #include <platform/mt_pmic.h>
     #include <string.h>
#else
    #include <linux/string.h>
    #if defined(BUILD_UBOOT)
        #include <asm/arch/mt_gpio.h>
    #else
        #include <linux/xlog.h>
        #include <mach/mt_gpio.h>
        #include <mach/mt_pm_ldo.h>
        #include <mach/upmu_common.h>
    #endif
#endif
#include "lcm_drv.h"
#include <cust_gpio_usage.h>

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (720)
#define FRAME_HEIGHT (1280)

#define LCM_ID (0x94)
#define GPIO_LCD_RST_EN      (GPIO28 | 0x80000000)
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};
LCM_DSI_MODE_SWITCH_CMD lcm_switch_mode1_cmd;
#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))
#define REGFLAG_DELAY                                           0xFC
#define REGFLAG_END_OF_TABLE                                0xFD   // END OF REGISTERS MARKER


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)									lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)				lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)   				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

struct LCM_setting_table {
unsigned cmd;
unsigned char count;
unsigned char para_list[64];
};

#define   LCM_DSI_CMD_MODE							0

static struct LCM_setting_table lcm_initialization_setting[] = {

			{0xB9,	3,	{0xFF,  	//  1
					 0x83,  	//  2
					 0x94,}},  	//  3
        {REGFLAG_DELAY, 10, {0}},

		//  BA-Set MIPI
		{0xBA,	2,	{0x33,  	//  1,    0x33 4-lane;0x32 3-lane;0x31 2-lane;
					 0x83,}},  	//  2
	    {REGFLAG_DELAY, 10, {0}},


		//  B1-Set Power
		{0xB1,	15,	{0x6e,  //    1, [8]=1 => DP_STB, DD_TU
					 0x10,  //    2
					 0x10,  //    3
					 0x37,  //    4, FS0 / FS1 ( 64 )
					 0x04,  //    5  VCL
					 0x11,  //    6, VSP
					 0xF1,  //    7, VSN
					 0x80,  //    8
					 0xd9,  //    9, VGH, E9
					 0x94,  //   10, VGL, 95
					 0x23,  //   11
					 0x80,  //   12
					 0xC0,  //   13
					 0xD2,  //   14
					 0x18,}},  //   15
		{REGFLAG_DELAY, 10, {0}},

		//  B2-Set Display
		{0xB2,	11,	{0x00,  //  1 ,NW
					 0x64,  //  2 ,NL
					 0x0E,  //  3 ,BP
					 0x0D,  //  4 ,FP
					 0x32,  //  5 ,SAP  //0x32
					 0x23,  //  6
					 0x08,  //  7
					 0x08,  //  8
					 0x1C,  //  9
					 0x4D,  //  10
					 0x00,}},  //  11     init_set
		{REGFLAG_DELAY, 20, {0}},


		//	B4-Set GIP Timing	Set CYC
		{0xB4,	12,	{0x00,  //   1, GEN_ON
					 0xFF,  //   2, GEN_OFF
					 0x03,  //   3, SPON
					 0x50,  //   4, SPOFF
					 0x03,  //   5, CON
					 0x50,  //   6, COFF   default 70
					 0x03,  //   7, CON1
					 0x50,  //   8, COFF1
					 0x01,  //   9, EQON1
					 0x6A,  //  10, EQON2
					 0x01,  //  11, S-ON
					 0x6A,}},  //  12, S-PFF
		{REGFLAG_DELAY, 10, {0}},

		// Set VDC
		{0xBC,	1,	{0x07}},  //  1
		{REGFLAG_DELAY, 10, {0}},

		// Set Power Option
		{0xBF,	3,	{0x41,  //  1
					 0x0E,  //  2
					 0x01,}},  //  3
        {REGFLAG_DELAY, 10, {0}},

		//  Set D3
		{0xD3,	30,	{0x00,0x07,0x00,0x00,0x00,0x10,0x00,0x32,0x10,0x05,
					 0x00,0x05,0x32,0x10,0x00,0x00,0x00,0x32,0x10,0x00,
					 0x00,0x00,0x36,0x03,0x09,0x09,0x37,0x00,0x00,0x37,}},
		{REGFLAG_DELAY, 5, {0}},

		//  Set GIP
		{0xD5,	44,	{0x02,0x03,0x00,0x01,0x06,0x07,0x04,0x05,0x20,0x21,
					 0x22,0x23,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
					 0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
					 0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x24,0x25,
					 0x19,0x19,0x18,0x18,}},
		{REGFLAG_DELAY, 5, {0}},

		//  Set D6
		{0xD6,	44,	{0x05,0x04,0x07,0x06,0x01,0x00,0x03,0x02,0x23,0x22,
					 0x21,0x20,0x18,0x18,0x18,0x18,0x18,0x18,0x58,0x58,
					 0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
					 0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x25,0x24,
					 0x18,0x18,0x19,0x19,}},
		{REGFLAG_DELAY, 10, {0}},

		{0xe0,	42,	{0x00,0x0f,0x17,0x38,0x3B,0x3F,0x28,0x50,0x08,0x0B,
				 0x0D,0x17,0x0F,0x12,0x15,0x13,0x14,0x08,0x12,0x16,
				 0x19,0x00,0x0f,0x17,0x38,0x3B,0x3F,0x28,0x50,0x08,
				 0x0B,0x0D,0x17,0x0F,0x12,0x15,0x13,0x14,0x08,0x12,
				 0x16,0x19}},


		//  Set Panel
		{0xCC,	1,	{0x01,}},
		{REGFLAG_DELAY, 5, {0}},

		//   Set C0
		{0xC0,	2,	{0x30,0x14,}},
		{REGFLAG_DELAY, 5, {0}},

		//   Set TCON Option
		{0xC7,	4,	{0x00,  //  1
					 0xC0,  //  2, [7:6]=TCON_OPT[23:22]
					 0x40,  //  3
					 0xC0,}},  //  4, TCON_OPT[7:6], GIP-Toggle=> OFF before SLP-OUT
	    {REGFLAG_DELAY, 5, {0}},

		//	B6-Set VCOM
		{0xB6,  2,	{0x43,0x43,}},
		{REGFLAG_DELAY, 5, {0}},

		{0x3A,  1,  {0x77}},
		{REGFLAG_DELAY, 5, {0}},

		{0x11,  0,  {0x00}},    //Sleep Out
        {REGFLAG_DELAY, 120, {0}},

        {0x29,  0,  {0x00}}, //Display On
        {REGFLAG_DELAY, 20, {0}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void lcm_register(void)
{
	unsigned int data_array[16];

  int v0 = 0;
data_array[0] = 0x43902;
  data_array[1] = 0x9483FFB9;
  dsi_set_cmdq(&data_array, 2, 1);
  MDELAY(5);
  data_array[0] = 0x33902;
  data_array[1] = 0x8373BA;
  dsi_set_cmdq(&data_array, 2, 1);
  MDELAY(1);
  data_array[0] = 0x103902;
  data_array[1] = 0x12126AB1;
  data_array[2] = 0xF111E424;
  data_array[3] = 0x239DE480;
  data_array[4] = 0x58D2C080;
  dsi_set_cmdq(&data_array, 5, 1);
  MDELAY(1);
  data_array[0] = 0xC3902;
  data_array[1] = 0x106400B2;
  data_array[2] = 0x81C1207;
  data_array[3] = 0x4D1C08;
  dsi_set_cmdq(&data_array, 4, 1);
  MDELAY(1);
  data_array[0] = 0xD3902;
  data_array[1] = 0x3FF00B4;
  data_array[2] = 0x35A035A;
  data_array[3] = 0x16A015A;
  data_array[4] = 0x6A;
  dsi_set_cmdq(&data_array, 5, 1);
  MDELAY(1);
  data_array[0] = 0x23902;
  data_array[1] = 0x55D2;
  dsi_set_cmdq(&data_array, 2, 1);
  MDELAY(1);
  data_array[0] = 0x43902;
  data_array[1] = 0x10E41BF;
  dsi_set_cmdq(&data_array, 2, 1);
  MDELAY(1);
  data_array[0] = 0x263902;
  data_array[1] = 0x600D3;
  data_array[2] = 0x81A40;
  data_array[3] = 0x71032;
  data_array[4] = 0xF155407;
  data_array[5] = 0x12020405;
  data_array[6] = 0x33070510;
  data_array[7] = 0x370B0B33;
  data_array[8] = 0x8070710;
  data_array[9] = 0xA000000;
  data_array[10] = 0x100;
  dsi_set_cmdq(&data_array, 11, 1);
  MDELAY(1);
  data_array[2] = 0x1B1A1A18;
  data_array[4] = 0x2010007;
  
  data_array[6] = 0x18181818;
  data_array[0] = 0x2D3902;
  data_array[12] = 0x18;
  data_array[1] = 0x181919D5;
  data_array[3] = 0x605041B;
  data_array[5] = 0x18212003;
  data_array[7] = 0x18181818;
  data_array[8] = 0x22181818;
  data_array[10] = 0x18181818;
  data_array[9] = 0x18181823;
  data_array[11] = 0x18181818;
  dsi_set_cmdq(&data_array, 13, 1);
  MDELAY(1);
  data_array[0] = 0x2D3902;
  data_array[12] = 0x18;
  data_array[1] = 0x191818D6;
  data_array[2] = 0x1B1A1A19;
  data_array[3] = 0x102031B;
  data_array[4] = 0x5060700;
  data_array[5] = 0x18222304;
  data_array[6] = 0x18181818;
  data_array[7] = 0x18181818;
  data_array[8] = 0x21181818;
  data_array[9] = 0x18181820;
  data_array[10] = 0x18181818;
  data_array[11] = 0x18181818;
  dsi_set_cmdq(&data_array, 13, 1);
  MDELAY(1);
  data_array[0] = 0x2C3902;
  data_array[1] = 0x171204E0;
  data_array[2] = 0x213E3330;
  data_array[3] = 0xC0A073C;
  data_array[4] = 0x13110D17;
  data_array[5] = 0x12081311;
  data_array[6] = 0x12041A16;
  data_array[7] = 0x3E333017;
  data_array[8] = 0xA073C21;
  data_array[9] = 0x110D170C;
  data_array[10] = 0x8131113;
  data_array[11] = 0x1A1612;
  dsi_set_cmdq(&data_array, 12, 1);
  MDELAY(1);
  data_array[0] = 0x33902;
  data_array[1] = 0x6565B6;
  dsi_set_cmdq(&data_array, 2, 1);
  MDELAY(1);
  data_array[0] = 0x23902;
  data_array[1] = 0x9CC;
  dsi_set_cmdq(&data_array, 2, 1);
  MDELAY(1);
  data_array[0] = 0x53902;
  data_array[1] = 0x40C000C7;
  data_array[2] = 0xC0;
  dsi_set_cmdq(&data_array, 3, 1);
  MDELAY(1);
  data_array[0] = 0x23902;
  data_array[1] = 0x87DF;
  dsi_set_cmdq(&data_array, 2, 1);
  MDELAY(1);
  data_array[0] = 0x23902;
  data_array[1] = 0x35;
  dsi_set_cmdq(&data_array, 2, 1);
  MDELAY(1);
  data_array[0] = 0x110500;
  dsi_set_cmdq(&data_array, 1, 1);
  MDELAY(120);
  data_array[0] = 0x290500;
  dsi_set_cmdq(&data_array, 1, 1);
  MDELAY(20);

}
static void push_table(struct LCM_setting_table *table, unsigned int
		count, unsigned char force_update)
{
	unsigned int i;

	for(i = 0; i < count; i++) {

		unsigned cmd;
		cmd = table[i].cmd;

		switch (cmd) {

		case REGFLAG_DELAY :
			MDELAY(table[i].count);
			break;

		case REGFLAG_END_OF_TABLE :
			break;

		default:
			dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}

}
static void lcm_init(void)
{
    SET_RESET_PIN(1);
    MDELAY(5);
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(120);

	lcm_register();
	//push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}
static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}




static unsigned int lcm_esd_check(void)
{
	int temp0=0,temp1=0,temp2=0,temp3=0;

	char  buffer[3];
	int   array[4];

	 array[0] = 0x43902;
  	array[1] = 0x9483FFB9;
  	dsi_set_cmdq(array, 2, 1);
  
  	array[0] = 0x33902;
  	array[1] = 0x8373BA;
  	dsi_set_cmdq(array, 2, 1);
  
  	array[0] = 0x130500;
  	dsi_set_cmdq(array, 1, 1);
  
  	array[0] = 0x290500;
  	dsi_set_cmdq(array, 1, 1);
  
  	array[0] = 0x43700;
  	dsi_set_cmdq(array, 1, 1);  
  	read_reg_v2(9, buffer, 4);
  
     	if ( buffer[0] == 0x80 && buffer[1] == 0x73 && buffer[2] == 6 && !buffer[3] ) {
    		return FALSE;
   	}
	
  	array[0] = 0x13700;
  	dsi_set_cmdq(array, 1, 1);  
  	read_reg_v2(217, buffer, 1);
	
 	temp0 = buffer[0];
	
  	array[0] = 0x13700;
  	dsi_set_cmdq(array, 1, 1);
  	read_reg_v2(10, buffer, 1);
  	temp1 = buffer[0];
	  
  	array[0] = 0x23700;
  	dsi_set_cmdq(array, 1, 1);
  	read_reg_v2(0x45, buffer, 2);
    	temp2 = buffer[0];  
  
  	if ( temp1 == 0x1C && temp0 == 0x80 && temp2 == 5 ) { //f#ck
      		return FALSE;
	}
	
	return TRUE;
}




static void lcm_get_params(LCM_PARAMS *params)
{
		memset(params, 0, sizeof(LCM_PARAMS));

		params->type   = LCM_TYPE_DSI;

    params->width  = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;

    // enable tearing-free
    params->dbi.te_mode                 = LCM_DBI_TE_MODE_DISABLED;
    params->dbi.te_edge_polarity        = LCM_POLARITY_RISING;

    params->dsi.mode   = SYNC_PULSE_VDO_MODE;

    // DSI
    /* Command mode setting */
    params->dsi.LANE_NUM                = LCM_THREE_LANE;
    //The following defined the fomat for data coming from LCD engine.
    params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
    params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
    params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    params->dsi.intermediat_buffer_num = 0;//because DSI/DPI HW design change, this parameters should be 0 when video mode in MT658X; or memory leakage

    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

    params->dsi.word_count=720*3;
    params->dsi.vertical_sync_active                = 2;
    params->dsi.vertical_backporch                  = 16;
    params->dsi.vertical_frontporch                 = 9;
    params->dsi.vertical_active_line                = FRAME_HEIGHT;
    params->dsi.horizontal_sync_active              = 18;
    params->dsi.horizontal_backporch                = 50;
    params->dsi.horizontal_frontporch               = 50;
    params->dsi.horizontal_active_pixel             = FRAME_WIDTH;


    // Video mode setting
    //params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //params->dsi.pll_select=1;
    //params->dsi.PLL_CLOCK = LCM_DSI_6589_PLL_CLOCK_253_5;//LCM_DSI_6589_PLL_CLOCK_240_5;//LCM_DSI_6589_PLL_CLOCK_227_5;//this value must be in MTK suggested table 227_5
	params->dsi.PLL_CLOCK = 230;

}


static void lcm_suspend(void)
{
	unsigned int data_array[16];

  data_array[0] = 0x280500; 
  dsi_set_cmdq(&data_array, 1, 1);

  data_array[0] = 0x100500; 
	dsi_set_cmdq(&data_array, 1, 1);


  MDELAY(120);

  
	SET_RESET_PIN(1);
    MDELAY(5);
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(120);



}

static void lcm_resume(void)
{
	lcm_init();
}


static unsigned int lcm_compare_id(void)
{
	

	return 1;
}

static unsigned int lcm_esd_recover(void)
{
	lcm_init();	
	return TRUE;
}



// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER HX8394D_TXD_HANNSTAR_IPS_HD_lcm_drv =
{
	.name           = "HX8394D_TXD_HANNSTAR_IPS_HD",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
     //.compare_id     = lcm_compare_id,
 #if (LCM_DSI_CMD_MODE)
    // .update         = lcm_update,
 #endif
//     .init_power        = lcm_init_power,
  //   .resume_power = lcm_resume_power,
    // .suspend_power = lcm_suspend_power,
	.esd_check	= lcm_esd_check,
	.esd_recover	= lcm_esd_recover,

     };
