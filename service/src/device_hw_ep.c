/*
** Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above
**     copyright notice, this list of conditions and the following
**     disclaimer in the documentation and/or other materials provided
**     with the distribution.
**   * Neither the name of The Linux Foundation nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
** WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
** ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
** BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
** BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
** WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
** OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
** IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**/

#define LOG_TAG "AGM: device"

#include <hw_intf_cmn_api.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <agm/utils.h>
#include <agm/device.h>

#ifdef DYNAMIC_LOG_ENABLED
#include <log_xml_parser.h>
#define LOG_MASK AGM_MOD_FILE_DEVICE_HW_EP
#include <log_utils.h>
#endif

#define DEV_ARG_SIZE                 21
#define DEV_VALUE_SIZE               61

#define CODEC_RX0 1
#define CODEC_TX0 1
#define CODEC_RX1 2
#define CODEC_TX1 2
#define CODEC_RX2 3
#define CODEC_TX2 3
#define CODEC_RX3 4
#define CODEC_TX3 4
#define CODEC_RX4 5
#define CODEC_TX4 5
#define CODEC_RX5 6
#define CODEC_TX5 6
#define CODEC_RX6 7
#define CODEC_RX7 8

#define SLIMBUS_DEVICE_1 0
#define SLIMBUS_DEVICE_2 1

static int populate_hw_ep_intf_idx(hw_ep_info_t *hw_ep_info, char *intf_idx)
{
    struct hw_ep_cdc_dma_i2s_tdm_config *cdc_dma_i2s_tdm_config;

    cdc_dma_i2s_tdm_config = &hw_ep_info->ep_config.cdc_dma_i2s_tdm_config;

    switch(hw_ep_info->intf) {
    case CODEC_DMA:
        if (!strcmp(intf_idx, "0"))
            cdc_dma_i2s_tdm_config->intf_idx = CODEC_RX0;
        else if (!strcmp(intf_idx, "1"))
            cdc_dma_i2s_tdm_config->intf_idx = CODEC_RX1;
        else if (!strcmp(intf_idx, "2"))
            cdc_dma_i2s_tdm_config->intf_idx = CODEC_RX2;
        else if (!strcmp(intf_idx, "3"))
            cdc_dma_i2s_tdm_config->intf_idx = CODEC_RX3;
        else if (!strcmp(intf_idx, "4"))
            cdc_dma_i2s_tdm_config->intf_idx = CODEC_RX4;
        else {
             AGM_LOGE("No matching intf_idx found\n");
             return -EINVAL;
        }
        break;
    case MI2S:
    case TDM:
    case AUXPCM:
        if (!strcmp(intf_idx, "PRIMARY"))
            cdc_dma_i2s_tdm_config->intf_idx = PCM_INTF_IDX_PRIMARY;
        else if (!strcmp(intf_idx, "SECONDARY"))
            cdc_dma_i2s_tdm_config->intf_idx = PCM_INTF_IDX_SECONDARY;
        else if (!strcmp(intf_idx, "TERTIARY"))
            cdc_dma_i2s_tdm_config->intf_idx = PCM_INTF_IDX_TERTIARY;
        else if (!strcmp(intf_idx, "QUATERNARY"))
            cdc_dma_i2s_tdm_config->intf_idx = PCM_INTF_IDX_QUATERNARY;
        else if(!strcmp(intf_idx, "QUINARY"))
            cdc_dma_i2s_tdm_config->intf_idx = PCM_INTF_IDX_QUINARY;
        else {
            AGM_LOGE("No matching intf_idx found\n");
            return -EINVAL;
        }
        break;
    default:
        AGM_LOGE("Unsupported HW endpoint %d\n", hw_ep_info->intf);
        return -EINVAL;
    }

    return 0;
}

static int populate_hw_ep_intf(hw_ep_info_t *hw_ep_info, char *intf)
{
    if (!strcmp(intf, "SLIM"))
        hw_ep_info->intf = SLIMBUS;
    else if (!strcmp(intf, "DISPLAY_PORT"))
        hw_ep_info->intf = DISPLAY_PORT;
    else if (!strcmp(intf, "USB_AUDIO"))
        hw_ep_info->intf = USB_AUDIO;
    else if (!strcmp(intf, "CODEC_DMA"))
        hw_ep_info->intf = CODEC_DMA;
    else if (!strcmp(intf, "MI2S"))
        hw_ep_info->intf = MI2S;
    else if (!strcmp(intf, "TDM"))
        hw_ep_info->intf = TDM;
    else if (!strcmp(intf, "AUXPCM"))
        hw_ep_info->intf = AUXPCM;
    else if (!strcmp(intf, "PCM_RT_PROXY"))
        hw_ep_info->intf = PCM_RT_PROXY;
    else {
        AGM_LOGE("No matching intf found\n");
        return -EINVAL;
    }
    return 0;
}

static int populate_hw_ep_direction(hw_ep_info_t *hw_ep_info, char *dir)
{
    if (!strcmp(dir, "RX"))
        hw_ep_info->dir = AUDIO_OUTPUT;
    else if (!strcmp(dir, "TX"))
        hw_ep_info->dir = AUDIO_INPUT;
    else {
        AGM_LOGE("No matching dir found\n");
        return -EINVAL;
    }
    return 0;
}

static int populate_pcm_rt_proxy_ep_info(hw_ep_info_t *hw_ep_info, char *value)
{
    char arg[DEV_ARG_SIZE] = {0};
    struct hw_ep_pcm_rt_proxy_config *pcm_rt_proxy_config;
    int ret = 0;

    sscanf(value, "%20[^-]-%60s", arg, value);
    ret = populate_hw_ep_direction(hw_ep_info, arg);
    if (ret) {
        AGM_LOGE("failed to parse direction\n");
        return ret;
    }

    pcm_rt_proxy_config = &hw_ep_info->ep_config.pcm_rt_proxy_config;
    sscanf(value, "%20[^-]-%60s", arg, value);
    pcm_rt_proxy_config->dev_id = atoi(arg);

    return ret;
}

static int populate_slim_dp_usb_ep_info(hw_ep_info_t *hw_ep_info, char *value)
{
    char dir[DEV_ARG_SIZE], arg[DEV_ARG_SIZE] = {0};
    char dev_id[DEV_ARG_SIZE] = {0};
    struct hw_ep_slimbus_config *slim_config;

    if (hw_ep_info->intf == SLIMBUS) {/* read_dev_id for SLIMBUS */
        slim_config = &hw_ep_info->ep_config.slim_config;
        sscanf(value, "%20[^-]-%60s", arg, value);
        strlcpy(dev_id, arg, strlen(arg)+1);
        if (!strcmp(dev_id, "DEV1"))
            slim_config->dev_id = SLIMBUS_DEVICE_1;
        else if (!strcmp(dev_id, "DEV2"))
            slim_config->dev_id = SLIMBUS_DEVICE_2;
        else {
            return -EINVAL;
        }

    }
    sscanf(value, "%20[^-]-%60s", arg, value);
    strlcpy(dir, arg, strlen(arg)+1);

    return populate_hw_ep_direction(hw_ep_info, dir);
}

static int populate_cdc_dma_i2s_tdm_pcm_ep_info(hw_ep_info_t *hw_ep_info, char *value)
{
    char lpaif_type[DEV_ARG_SIZE];
    char intf_idx[DEV_ARG_SIZE], dir[DEV_ARG_SIZE];
    char arg[DEV_ARG_SIZE] = {0};

    struct hw_ep_cdc_dma_i2s_tdm_config *cdc_dma_i2s_tdm_config;
    int ret = 0;

    cdc_dma_i2s_tdm_config = &hw_ep_info->ep_config.cdc_dma_i2s_tdm_config;

    sscanf(value, "%20[^-]-%60s", arg, value);
    strlcpy(lpaif_type, arg, strlen(arg)+1);
    sscanf(value, "%20[^-]-%60s", arg, value);
    strlcpy(dir, arg, strlen(arg)+1);
    sscanf(value, "%20[^-]-%60s", arg, value);
    strlcpy(intf_idx, arg, strlen(arg)+1);

   if (!strcmp(lpaif_type, "LPAIF"))
        cdc_dma_i2s_tdm_config->lpaif_type = LPAIF;
    else if (!strcmp(lpaif_type, "LPAIF_RXTX"))
        cdc_dma_i2s_tdm_config->lpaif_type = LPAIF_RXTX;
    else if (!strcmp(lpaif_type, "LPAIF_WSA"))
        cdc_dma_i2s_tdm_config->lpaif_type = LPAIF_WSA;
    else if (!strcmp(lpaif_type, "LPAIF_VA"))
        cdc_dma_i2s_tdm_config->lpaif_type = LPAIF_VA;
    else if (!strcmp(lpaif_type, "LPAIF_AXI"))
        cdc_dma_i2s_tdm_config->lpaif_type = LPAIF_AXI;
    else {
        AGM_LOGE("No matching lpaif_type found\n");
        return -EINVAL;
    }

    ret = populate_hw_ep_direction(hw_ep_info, dir);
    if (ret)
        return ret;

    return populate_hw_ep_intf_idx(hw_ep_info, intf_idx);
}

int populate_device_hw_ep_info(struct device_obj *dev_obj)
{
    char intf[DEV_ARG_SIZE];
    char arg[DEV_ARG_SIZE] = {0};
    char value[DEV_VALUE_SIZE] = {0};
    int ret = 0;

    if (dev_obj == NULL) {
        AGM_LOGE("Invalid device object\n");
        return -EINVAL;
    }

    sscanf(dev_obj->name, "%20[^-]-%60s", arg, value);
    strlcpy(intf, arg, strlen(arg)+1);

    ret = populate_hw_ep_intf(&dev_obj->hw_ep_info, intf);
    if (ret)
        return ret;

    switch(dev_obj->hw_ep_info.intf) {
    case CODEC_DMA:
    case MI2S:
    case TDM:
    case AUXPCM:
        return populate_cdc_dma_i2s_tdm_pcm_ep_info(&dev_obj->hw_ep_info, value);
    case SLIMBUS:
    case DISPLAY_PORT:
    case USB_AUDIO:
        return populate_slim_dp_usb_ep_info(&dev_obj->hw_ep_info, value);
    case PCM_RT_PROXY:
        return populate_pcm_rt_proxy_ep_info(&dev_obj->hw_ep_info, value);
    default:
        AGM_LOGE("Unsupported interface name %s\n", __func__, dev_obj->name);
        return -EINVAL;
    }
}
