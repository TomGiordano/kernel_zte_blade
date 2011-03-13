/*------------------------------------------------------------------------------ */
/* <copyright file="ioctl.c" company="Atheros"> */
/*    Copyright (c) 2004-2009 Atheros Corporation.  All rights reserved. */
/*  */
/* This program is free software; you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License version 2 as */
/* published by the Free Software Foundation; */
/* */
/* Software distributed under the License is distributed on an "AS */
/* IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or */
/* implied. See the License for the specific language governing */
/* rights and limitations under the License. */
/* */
/* */
/*------------------------------------------------------------------------------ */
/*============================================================================== */
/* Author(s): ="Atheros" */
/*============================================================================== */

#include "ar6000_drv.h"
#include "ieee80211_ioctl.h"
#include "ar6kap_common.h"

static A_UINT8 bcast_mac[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static A_UINT8 null_mac[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
extern unsigned int wmitimeout;
extern A_WAITQUEUE_HEAD arEvent;
extern int tspecCompliance;
extern int bmienable;
extern int bypasswmi;

/* ATHENV */
#ifdef ANDROID_ENV
extern int chan_num;
#endif
/* ATHENV */

static int
ar6000_ioctl_get_roam_tbl(struct net_device *dev, struct ifreq *rq)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);

    if (ar->arWmiReady == FALSE) {
        return -EIO;
    }

    if(wmi_get_roam_tbl_cmd(ar->arWmi) != A_OK) {
        return -EIO;
    }

    return 0;
}

static int
ar6000_ioctl_get_roam_data(struct net_device *dev, struct ifreq *rq)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);

    if (ar->arWmiReady == FALSE) {
        return -EIO;
    }


    /* currently assume only roam times are required */
    if(wmi_get_roam_data_cmd(ar->arWmi, ROAM_DATA_TIME) != A_OK) {
        return -EIO;
    }


    return 0;
}

static int
ar6000_ioctl_set_roam_ctrl(struct net_device *dev, char *userdata)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    WMI_SET_ROAM_CTRL_CMD cmd;
    A_UINT8 size = sizeof(cmd);

    if (ar->arWmiReady == FALSE) {
        return -EIO;
    }


    if (copy_from_user(&cmd, userdata, size)) {
        return -EFAULT;
    }

    if (cmd.roamCtrlType == WMI_SET_HOST_BIAS) {
        if (cmd.info.bssBiasInfo.numBss > 1) {
            size += (cmd.info.bssBiasInfo.numBss - 1) * sizeof(WMI_BSS_BIAS);
        }
    }

    if (copy_from_user(&cmd, userdata, size)) {
        return -EFAULT;
    }

    if(wmi_set_roam_ctrl_cmd(ar->arWmi, &cmd, size) != A_OK) {
        return -EIO;
    }

    return 0;
}

static int
ar6000_ioctl_set_powersave_timers(struct net_device *dev, char *userdata)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    WMI_POWERSAVE_TIMERS_POLICY_CMD cmd;
    A_UINT8 size = sizeof(cmd);

    if (ar->arWmiReady == FALSE) {
        return -EIO;
    }

    if (copy_from_user(&cmd, userdata, size)) {
        return -EFAULT;
    }

    if (copy_from_user(&cmd, userdata, size)) {
        return -EFAULT;
    }

    if(wmi_set_powersave_timers_cmd(ar->arWmi, &cmd, size) != A_OK) {
        return -EIO;
    }

    return 0;
}

static int
ar6000_ioctl_set_wmm(struct net_device *dev, struct ifreq *rq)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    WMI_SET_WMM_CMD cmd;
    A_STATUS ret;

    if ((dev->flags & IFF_UP) != IFF_UP) {
        return -EIO;
    }
    if (ar->arWmiReady == FALSE) {
        return -EIO;
    }

    if (copy_from_user(&cmd, (char *)((unsigned int*)rq->ifr_data + 1),
                                sizeof(cmd)))
    {
        return -EFAULT;
    }

    if (cmd.status == WMI_WMM_ENABLED) {
        ar->arWmmEnabled = TRUE;
    } else {
        ar->arWmmEnabled = FALSE;
    }

    ret = wmi_set_wmm_cmd(ar->arWmi, cmd.status);

    switch (ret) {
        case A_OK:
            return 0;
        case A_EBUSY :
            return -EBUSY;
        case A_NO_MEMORY:
            return -ENOMEM;
        case A_EINVAL:
        default:
            return -EFAULT;
    }
}

static int
ar6000_ioctl_set_txop(struct net_device *dev, struct ifreq *rq)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    WMI_SET_WMM_TXOP_CMD cmd;
    A_STATUS ret;

    if ((dev->flags & IFF_UP) != IFF_UP) {
        return -EIO;
    }
    if (ar->arWmiReady == FALSE) {
        return -EIO;
    }

    if (copy_from_user(&cmd, (char *)((unsigned int*)rq->ifr_data + 1),
                                sizeof(cmd)))
    {
        return -EFAULT;
    }

    ret = wmi_set_wmm_txop(ar->arWmi, cmd.txopEnable);

    switch (ret) {
        case A_OK:
            return 0;
        case A_EBUSY :
            return -EBUSY;
        case A_NO_MEMORY:
            return -ENOMEM;
        case A_EINVAL:
        default:
            return -EFAULT;
    }
}

static int
ar6000_ioctl_get_rd(struct net_device *dev, struct ifreq *rq)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    A_STATUS ret = 0;

    if ((dev->flags & IFF_UP) != IFF_UP || ar->arWmiReady == FALSE) {
        return -EIO;
    }

    if(copy_to_user((char *)((unsigned int*)rq->ifr_data + 1),
                            &ar->arRegCode, sizeof(ar->arRegCode)))
        ret = -EFAULT;

    return ret;
}

static int
ar6000_ioctl_set_country(struct net_device *dev, struct ifreq *rq)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    WMI_SET_COUNTRY_CMD cmd;
    A_STATUS ret;

    if ((dev->flags & IFF_UP) != IFF_UP) {
        return -EIO;
    }
    if (ar->arWmiReady == FALSE) {
        return -EIO;
    }

    if (copy_from_user(&cmd, (char *)((unsigned int*)rq->ifr_data + 1),
                                sizeof(cmd)))
    {
        return -EFAULT;
    }

    if (AP_NETWORK == ar->arNetworkType) {
        ar->ap_profile_flag = 1; /* There is a change in profile */
    }

    ret = wmi_set_country(ar->arWmi, cmd.countryCode);
    A_MEMCPY(ar->country_code, cmd.countryCode, 3);

    switch (ret) {
        case A_OK:
            return 0;
        case A_EBUSY :
            return -EBUSY;
        case A_NO_MEMORY:
            return -ENOMEM;
        case A_EINVAL:
        default:
            return -EFAULT;
    }
}


/* Get power mode command */
static int
ar6000_ioctl_get_power_mode(struct net_device *dev, struct ifreq *rq)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    WMI_POWER_MODE_CMD power_mode;
    int ret = 0;

    if (ar->arWmiReady == FALSE) {
        return -EIO;
    }

    power_mode.powerMode = wmi_get_power_mode_cmd(ar->arWmi);
    if (copy_to_user(rq->ifr_data, &power_mode, sizeof(WMI_POWER_MODE_CMD))) {
        ret = -EFAULT;
    }

    return ret;
}


static int
ar6000_ioctl_set_channelParams(struct net_device *dev, struct ifreq *rq)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    WMI_CHANNEL_PARAMS_CMD cmd, *cmdp;
    int ret = 0;

    if (ar->arWmiReady == FALSE) {
        return -EIO;
    }


    if (copy_from_user(&cmd, rq->ifr_data, sizeof(cmd))) {
        return -EFAULT;
    }

    if( (ar->arNextMode == AP_NETWORK) && (cmd.numChannels || cmd.scanParam) ) {
        A_PRINTF("ERROR: Only wmode is allowed in AP mode\n");
        return -EIO;
    }

/* ATHENV */
#ifdef ANDROID_ENV
    if ((ar->arNextMode != AP_NETWORK) && (cmd.numChannels == chan_num)) {
        AR_DEBUG_PRINTF("AR6000: configure to the same channel number [0x%x]\n", cmd.numChannels);
        return 0;
    }
#endif
/* ATHENV */

    if (cmd.numChannels > 1) {
        cmdp = A_MALLOC(130);
        if (copy_from_user(cmdp, rq->ifr_data,
                           sizeof (*cmdp) +
                           ((cmd.numChannels - 1) * sizeof(A_UINT16))))
        {
            kfree(cmdp);
            return -EFAULT;
        }
    } else {
        cmdp = &cmd;
    }

    if ((ar->arPhyCapability == WMI_11G_CAPABILITY) &&
        ((cmdp->phyMode == WMI_11A_MODE) || (cmdp->phyMode == WMI_11AG_MODE)))
    {
        ret = -EINVAL;
    }

/* ATHENV */
#ifdef ANDROID_ENV
    AR6000_SPIN_LOCK(&ar->arLock, 0);
    if (!ret && (ar->arConnected == TRUE || ar->arConnectPending == TRUE)) {
        AR6000_SPIN_UNLOCK(&ar->arLock, 0);
        if (ar->arNextMode != AP_NETWORK)
        wmi_disconnect_cmd(ar->arWmi);
    } else {
        AR6000_SPIN_UNLOCK(&ar->arLock, 0);
    }
    wmi_free_allnodes(ar->arWmi);
#endif
/* ATHENV */

    if (!ret &&
        (wmi_set_channelParams_cmd(ar->arWmi, cmdp->scanParam, cmdp->phyMode,
                                   cmdp->numChannels, cmdp->channelList)
         != A_OK))
    {
        ret = -EIO;
    }
/* ATHENV */
#ifdef ANDROID_ENV
    else
        chan_num = cmdp->numChannels;
#endif
/* ATHENV */

    if (cmd.numChannels > 1) {
        kfree(cmdp);
    }

    ar->ap_wmode = cmdp->phyMode;
    /* Set the profile change flag to allow a commit cmd */
    ar->ap_profile_flag = 1;

    return ret;
}


static int
ar6000_ioctl_set_snr_threshold(struct net_device *dev, struct ifreq *rq)
{

    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    WMI_SNR_THRESHOLD_PARAMS_CMD cmd;
    int ret = 0;

    if (ar->arWmiReady == FALSE) {
        return -EIO;
    }

    if (copy_from_user(&cmd, rq->ifr_data, sizeof(cmd))) {
        return -EFAULT;
    }

    if( wmi_set_snr_threshold_params(ar->arWmi, &cmd) != A_OK ) {
        ret = -EIO;
    }

    return ret;
}

static int
ar6000_ioctl_set_rssi_threshold(struct net_device *dev, struct ifreq *rq)
{
#define SWAP_THOLD(thold1, thold2) do { \
    USER_RSSI_THOLD tmpThold;           \
    tmpThold.tag = thold1.tag;          \
    tmpThold.rssi = thold1.rssi;        \
    thold1.tag = thold2.tag;            \
    thold1.rssi = thold2.rssi;          \
    thold2.tag = tmpThold.tag;          \
    thold2.rssi = tmpThold.rssi;        \
} while (0)

    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    WMI_RSSI_THRESHOLD_PARAMS_CMD cmd;
    USER_RSSI_PARAMS rssiParams;
    A_INT32 i, j;
    int ret = 0;

    if (ar->arWmiReady == FALSE) {
        return -EIO;
    }

    if (copy_from_user((char *)&rssiParams, (char *)((unsigned int *)rq->ifr_data + 1), sizeof(USER_RSSI_PARAMS))) {
        return -EFAULT;
    }
    cmd.weight = rssiParams.weight;
    cmd.pollTime = rssiParams.pollTime;

    A_MEMCPY(ar->rssi_map, &rssiParams.tholds, sizeof(ar->rssi_map));
    /*
     *  only 6 elements, so use bubble sorting, in ascending order
     */
    for (i = 5; i > 0; i--) {
        for (j = 0; j < i; j++) { /* above tholds */
            if (ar->rssi_map[j+1].rssi < ar->rssi_map[j].rssi) {
                SWAP_THOLD(ar->rssi_map[j+1], ar->rssi_map[j]);
            } else if (ar->rssi_map[j+1].rssi == ar->rssi_map[j].rssi) {
                return EFAULT;
            }
        }
    }
    for (i = 11; i > 6; i--) {
        for (j = 6; j < i; j++) { /* below tholds */
            if (ar->rssi_map[j+1].rssi < ar->rssi_map[j].rssi) {
                SWAP_THOLD(ar->rssi_map[j+1], ar->rssi_map[j]);
            } else if (ar->rssi_map[j+1].rssi == ar->rssi_map[j].rssi) {
                return EFAULT;
            }
        }
    }

#ifdef DEBUG
    for (i = 0; i < 12; i++) {
        AR_DEBUG2_PRINTF("thold[%d].tag: %d, thold[%d].rssi: %d \n",
                i, ar->rssi_map[i].tag, i, ar->rssi_map[i].rssi);
    }
#endif
    cmd.thresholdAbove1_Val = ar->rssi_map[0].rssi;
    cmd.thresholdAbove2_Val = ar->rssi_map[1].rssi;
    cmd.thresholdAbove3_Val = ar->rssi_map[2].rssi;
    cmd.thresholdAbove4_Val = ar->rssi_map[3].rssi;
    cmd.thresholdAbove5_Val = ar->rssi_map[4].rssi;
    cmd.thresholdAbove6_Val = ar->rssi_map[5].rssi;
    cmd.thresholdBelow1_Val = ar->rssi_map[6].rssi;
    cmd.thresholdBelow2_Val = ar->rssi_map[7].rssi;
    cmd.thresholdBelow3_Val = ar->rssi_map[8].rssi;
    cmd.thresholdBelow4_Val = ar->rssi_map[9].rssi;
    cmd.thresholdBelow5_Val = ar->rssi_map[10].rssi;
    cmd.thresholdBelow6_Val = ar->rssi_map[11].rssi;

    if( wmi_set_rssi_threshold_params(ar->arWmi, &cmd) != A_OK ) {
        ret = -EIO;
    }

    return ret;
}

static int
ar6000_ioctl_set_lq_threshold(struct net_device *dev, struct ifreq *rq)
{

    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    WMI_LQ_THRESHOLD_PARAMS_CMD cmd;
    int ret = 0;

    if (ar->arWmiReady == FALSE) {
        return -EIO;
    }

    if (copy_from_user(&cmd, (char *)((unsigned int *)rq->ifr_data + 1), sizeof(cmd))) {
        return -EFAULT;
    }

    if( wmi_set_lq_threshold_params(ar->arWmi, &cmd) != A_OK ) {
        ret = -EIO;
    }

    return ret;
}


static int
ar6000_ioctl_set_probedSsid(struct net_device *dev, struct ifreq *rq)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    WMI_PROBED_SSID_CMD cmd;
    int ret = 0;

    if (ar->arWmiReady == FALSE) {
        return -EIO;
    }

    if (copy_from_user(&cmd, rq->ifr_data, sizeof(cmd))) {
        return -EFAULT;
    }

    if (wmi_probedSsid_cmd(ar->arWmi, cmd.entryIndex, cmd.flag, cmd.ssidLength,
                                  cmd.ssid) != A_OK)
    {
        ret = -EIO;
    }

    return ret;
}

static int
ar6000_ioctl_set_badAp(struct net_device *dev, struct ifreq *rq)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    WMI_ADD_BAD_AP_CMD cmd;
    int ret = 0;

    if (ar->arWmiReady == FALSE) {
        return -EIO;
    }


    if (copy_from_user(&cmd, rq->ifr_data, sizeof(cmd))) {
        return -EFAULT;
    }

    if (cmd.badApIndex > WMI_MAX_BAD_AP_INDEX) {
        return -EIO;
    }

    if (A_MEMCMP(cmd.bssid, null_mac, AR6000_ETH_ADDR_LEN) == 0) {
        /*
         * This is a delete badAP.
         */
        if (wmi_deleteBadAp_cmd(ar->arWmi, cmd.badApIndex) != A_OK) {
            ret = -EIO;
        }
    } else {
        if (wmi_addBadAp_cmd(ar->arWmi, cmd.badApIndex, cmd.bssid) != A_OK) {
            ret = -EIO;
        }
    }

    return ret;
}

static int
ar6000_ioctl_create_qos(struct net_device *dev, struct ifreq *rq)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    WMI_CREATE_PSTREAM_CMD cmd;
    A_STATUS ret;

    if (ar->arWmiReady == FALSE) {
        return -EIO;
    }


    if (copy_from_user(&cmd, rq->ifr_data, sizeof(cmd))) {
        return -EFAULT;
    }

    ret = wmi_verify_tspec_params(&cmd, tspecCompliance);
    if (ret == A_OK)
        ret = wmi_create_pstream_cmd(ar->arWmi, &cmd);

    switch (ret) {
        case A_OK:
            return 0;
        case A_EBUSY :
            return -EBUSY;
        case A_NO_MEMORY:
            return -ENOMEM;
        case A_EINVAL:
        default:
            return -EFAULT;
    }
}

static int
ar6000_ioctl_delete_qos(struct net_device *dev, struct ifreq *rq)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    WMI_DELETE_PSTREAM_CMD cmd;
    int ret = 0;

    if (ar->arWmiReady == FALSE) {
        return -EIO;
    }

    if (copy_from_user(&cmd, rq->ifr_data, sizeof(cmd))) {
        return -EFAULT;
    }

    ret = wmi_delete_pstream_cmd(ar->arWmi, cmd.trafficClass, cmd.tsid);

    switch (ret) {
        case A_OK:
            return 0;
        case A_EBUSY :
            return -EBUSY;
        case A_NO_MEMORY:
            return -ENOMEM;
        case A_EINVAL:
        default:
            return -EFAULT;
    }
}

static int
ar6000_ioctl_get_qos_queue(struct net_device *dev, struct ifreq *rq)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    struct ar6000_queuereq qreq;
    int ret = 0;

    if (ar->arWmiReady == FALSE) {
        return -EIO;
    }

    if( copy_from_user(&qreq, rq->ifr_data,
                  sizeof(struct ar6000_queuereq)))
        return -EFAULT;

    qreq.activeTsids = wmi_get_mapped_qos_queue(ar->arWmi, qreq.trafficClass);

    if (copy_to_user(rq->ifr_data, &qreq,
                 sizeof(struct ar6000_queuereq)))
    {
        ret = -EFAULT;
    }

    return ret;
}

#ifdef CONFIG_HOST_TCMD_SUPPORT
static A_STATUS
ar6000_ioctl_tcmd_get_rx_report(struct net_device *dev,
                                 struct ifreq *rq, A_UINT8 *data, A_UINT32 len)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    A_UINT32    buf[5];
    int ret = 0;

    if (ar->bIsDestroyProgress) {
        return -EBUSY;
    }

    if (ar->arWmiReady == FALSE) {
        return -EIO;
    }

    if (down_interruptible(&ar->arSem)) {
        return -ERESTARTSYS;
    }

    if (ar->bIsDestroyProgress) {
        up(&ar->arSem);
        return -EBUSY;
    }

    ar->tcmdRxReport = 0;
    if (wmi_test_cmd(ar->arWmi, data, len) != A_OK) {
        up(&ar->arSem);
        return -EIO;
    }

    wait_event_interruptible_timeout(arEvent, ar->tcmdRxReport != 0, wmitimeout * HZ);

    if (signal_pending(current)) {
        ret = -EINTR;
    }

    buf[0] = ar->tcmdRxTotalPkt;
    buf[1] = ar->tcmdRxRssi;
    buf[2] = ar->tcmdRxcrcErrPkt;
    buf[3] = ar->tcmdRxsecErrPkt;
    buf[4] = ar->tcmdRxNoiseFloor;
    if (!ret && copy_to_user(rq->ifr_data, buf, sizeof(buf))) {
        ret = -EFAULT;
    }

    up(&ar->arSem);

    return ret;
}

static A_STATUS
ar6000_ioctl_tcmd_get_mac(struct net_device *dev,
                                 struct ifreq *rq, A_UINT8 *data, A_UINT32 len)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    A_UINT8    buf[6];
    int ret = 0;

    if (ar->bIsDestroyProgress) {
        return -EBUSY;
    }

    if (ar->arWmiReady == FALSE) {
        return -EIO;
    }

    if (down_interruptible(&ar->arSem)) {
        return -ERESTARTSYS;
    }

    if (ar->bIsDestroyProgress) {
        up(&ar->arSem);
        return -EBUSY;
    }

    buf[0] = ar->arNetDev->dev_addr[0];
    buf[1] = ar->arNetDev->dev_addr[1];
    buf[2] = ar->arNetDev->dev_addr[2];
    buf[3] = ar->arNetDev->dev_addr[3];
    buf[4] = ar->arNetDev->dev_addr[4];
    buf[5] = ar->arNetDev->dev_addr[5];
    if (!ret && copy_to_user(rq->ifr_data, buf, sizeof(buf))) {
        ret = -EFAULT;
    }
    printk("ar6000_ioctl_tcmd_get_mac : %x:%x:%x:%x:%x:%x \n",
    buf[0],
    buf[1],
    buf[2],
    buf[3],
    buf[4],
    buf[5]);


    up(&ar->arSem);

    return ret;
}

void
ar6000_tcmd_rx_report_event(void *devt, A_UINT8 * results, int len)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)devt;
    TCMD_CONT_RX * rx_rep = (TCMD_CONT_RX *)results;

    ar->tcmdRxTotalPkt = rx_rep->u.report.totalPkt;
    ar->tcmdRxRssi = rx_rep->u.report.rssiInDBm;
    ar->tcmdRxcrcErrPkt = rx_rep->u.report.crcErrPkt;
    ar->tcmdRxsecErrPkt = rx_rep->u.report.secErrPkt;
    ar->tcmdRxNoiseFloor = rx_rep->u.report.noiseFloor;
    ar->tcmdRxReport = 1;

    wake_up(&arEvent);
}
#endif /* CONFIG_HOST_TCMD_SUPPORT*/

static int
ar6000_ioctl_set_error_report_bitmask(struct net_device *dev, struct ifreq *rq)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    WMI_TARGET_ERROR_REPORT_BITMASK cmd;
    int ret = 0;

    if (ar->arWmiReady == FALSE) {
        return -EIO;
    }

    if (copy_from_user(&cmd, rq->ifr_data, sizeof(cmd))) {
        return -EFAULT;
    }

    ret = wmi_set_error_report_bitmask(ar->arWmi, cmd.bitmask);

    return  (ret==0 ? ret : -EINVAL);
}

static int
ar6000_clear_target_stats(struct net_device *dev)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    TARGET_STATS *pStats = &ar->arTargetStats;
    int ret = 0;

    if (ar->arWmiReady == FALSE) {
       return -EIO;
    }
    AR6000_SPIN_LOCK(&ar->arLock, 0);
    A_MEMZERO(pStats, sizeof(TARGET_STATS));
    AR6000_SPIN_UNLOCK(&ar->arLock, 0);
    return ret;
}

static int
ar6000_ioctl_get_target_stats(struct net_device *dev, struct ifreq *rq)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    TARGET_STATS_CMD cmd;
    TARGET_STATS *pStats = &ar->arTargetStats;
    int ret = 0;

    if (ar->bIsDestroyProgress) {
        return -EBUSY;
    }
    if (ar->arWmiReady == FALSE) {
        return -EIO;
    }
    if (copy_from_user(&cmd, rq->ifr_data, sizeof(cmd))) {
        return -EFAULT;
    }
    if (down_interruptible(&ar->arSem)) {
        return -ERESTARTSYS;
    }
    if (ar->bIsDestroyProgress) {
        up(&ar->arSem);
        return -EBUSY;
    }

    ar->statsUpdatePending = TRUE;

    if(wmi_get_stats_cmd(ar->arWmi) != A_OK) {
        up(&ar->arSem);
        return -EIO;
    }

    wait_event_interruptible_timeout(arEvent, ar->statsUpdatePending == FALSE, wmitimeout * HZ);

    if (signal_pending(current)) {
        ret = -EINTR;
    }

    if (!ret && copy_to_user(rq->ifr_data, pStats, sizeof(*pStats))) {
        ret = -EFAULT;
    }

    if (cmd.clearStats == 1) {
        ret = ar6000_clear_target_stats(dev);
    }

    up(&ar->arSem);

    return ret;
}

static int
ar6000_ioctl_set_access_params(struct net_device *dev, struct ifreq *rq)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    WMI_SET_ACCESS_PARAMS_CMD cmd;
    int ret = 0;

    if (ar->arWmiReady == FALSE) {
        return -EIO;
    }

    if (copy_from_user(&cmd, rq->ifr_data, sizeof(cmd))) {
        return -EFAULT;
    }

    if (wmi_set_access_params_cmd(ar->arWmi, cmd.txop, cmd.eCWmin, cmd.eCWmax,
                                  cmd.aifsn) == A_OK)
    {
        ret = 0;
    } else {
        ret = -EINVAL;
    }

    return (ret);
}

static int
ar6000_ioctl_set_disconnect_timeout(struct net_device *dev, struct ifreq *rq)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    WMI_DISC_TIMEOUT_CMD cmd;
    int ret = 0;

    if (ar->arWmiReady == FALSE) {
        return -EIO;
    }

    if (copy_from_user(&cmd, rq->ifr_data, sizeof(cmd))) {
        return -EFAULT;
    }

    if (wmi_disctimeout_cmd(ar->arWmi, cmd.disconnectTimeout) == A_OK)
    {
        ret = 0;
    } else {
        ret = -EINVAL;
    }

    return (ret);
}

static int
ar6000_xioctl_set_voice_pkt_size(struct net_device *dev, char * userdata)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    WMI_SET_VOICE_PKT_SIZE_CMD cmd;
    int ret = 0;

    if (ar->arWmiReady == FALSE) {
        return -EIO;
    }

    if (copy_from_user(&cmd, userdata, sizeof(cmd))) {
        return -EFAULT;
    }

    if (wmi_set_voice_pkt_size_cmd(ar->arWmi, cmd.voicePktSize) == A_OK)
    {
        ret = 0;
    } else {
        ret = -EINVAL;
    }


    return (ret);
}

static int
ar6000_xioctl_set_max_sp_len(struct net_device *dev, char * userdata)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    WMI_SET_MAX_SP_LEN_CMD cmd;
    int ret = 0;

    if (ar->arWmiReady == FALSE) {
        return -EIO;
    }

    if (copy_from_user(&cmd, userdata, sizeof(cmd))) {
        return -EFAULT;
    }

    if (wmi_set_max_sp_len_cmd(ar->arWmi, cmd.maxSPLen) == A_OK)
    {
        ret = 0;
    } else {
        ret = -EINVAL;
    }

    return (ret);
}


static int
ar6000_xioctl_set_bt_status_cmd(struct net_device *dev, char * userdata)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    WMI_SET_BT_STATUS_CMD cmd;
    int ret = 0;

    if (ar->arWmiReady == FALSE) {
        return -EIO;
    }

    if (copy_from_user(&cmd, userdata, sizeof(cmd))) {
        return -EFAULT;
    }

    if (wmi_set_bt_status_cmd(ar->arWmi, cmd.streamType, cmd.status) == A_OK)
    {
        ret = 0;
    } else {
        ret = -EINVAL;
    }

    return (ret);
}

static int
ar6000_xioctl_set_bt_params_cmd(struct net_device *dev, char * userdata)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    WMI_SET_BT_PARAMS_CMD cmd;
    int ret = 0;

    if (ar->arWmiReady == FALSE) {
        return -EIO;
    }

    if (copy_from_user(&cmd, userdata, sizeof(cmd))) {
        return -EFAULT;
    }

    if (wmi_set_bt_params_cmd(ar->arWmi, &cmd) == A_OK)
    {
        ret = 0;
    } else {
        ret = -EINVAL;
    }

    return (ret);
}

#ifdef CONFIG_HOST_GPIO_SUPPORT
struct ar6000_gpio_intr_wait_cmd_s  gpio_intr_results;
/* gpio_reg_results and gpio_data_available are protected by arSem */
static struct ar6000_gpio_register_cmd_s gpio_reg_results;
static A_BOOL gpio_data_available; /* Requested GPIO data available */
static A_BOOL gpio_intr_available; /* GPIO interrupt info available */
static A_BOOL gpio_ack_received;   /* GPIO ack was received */

/* Host-side initialization for General Purpose I/O support */
void ar6000_gpio_init(void)
{
    gpio_intr_available = FALSE;
    gpio_data_available = FALSE;
    gpio_ack_received   = FALSE;
}

/*
 * Called when a GPIO interrupt is received from the Target.
 * intr_values shows which GPIO pins have interrupted.
 * input_values shows a recent value of GPIO pins.
 */
void
ar6000_gpio_intr_rx(A_UINT32 intr_mask, A_UINT32 input_values)
{
    gpio_intr_results.intr_mask = intr_mask;
    gpio_intr_results.input_values = input_values;
    *((volatile A_BOOL *)&gpio_intr_available) = TRUE;
    wake_up(&arEvent);
}

/*
 * This is called when a response is received from the Target
 * for a previous or ar6000_gpio_input_get or ar6000_gpio_register_get
 * call.
 */
void
ar6000_gpio_data_rx(A_UINT32 reg_id, A_UINT32 value)
{
    gpio_reg_results.gpioreg_id = reg_id;
    gpio_reg_results.value = value;
    *((volatile A_BOOL *)&gpio_data_available) = TRUE;
    wake_up(&arEvent);
}

/*
 * This is called when an acknowledgement is received from the Target
 * for a previous or ar6000_gpio_output_set or ar6000_gpio_register_set
 * call.
 */
void
ar6000_gpio_ack_rx(void)
{
    gpio_ack_received = TRUE;
    wake_up(&arEvent);
}

A_STATUS
ar6000_gpio_output_set(struct net_device *dev,
                       A_UINT32 set_mask,
                       A_UINT32 clear_mask,
                       A_UINT32 enable_mask,
                       A_UINT32 disable_mask)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);

    gpio_ack_received = FALSE;
    return wmi_gpio_output_set(ar->arWmi,
                set_mask, clear_mask, enable_mask, disable_mask);
}

static A_STATUS
ar6000_gpio_input_get(struct net_device *dev)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);

    *((volatile A_BOOL *)&gpio_data_available) = FALSE;
    return wmi_gpio_input_get(ar->arWmi);
}

static A_STATUS
ar6000_gpio_register_set(struct net_device *dev,
                         A_UINT32 gpioreg_id,
                         A_UINT32 value)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);

    gpio_ack_received = FALSE;
    return wmi_gpio_register_set(ar->arWmi, gpioreg_id, value);
}

static A_STATUS
ar6000_gpio_register_get(struct net_device *dev,
                         A_UINT32 gpioreg_id)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);

    *((volatile A_BOOL *)&gpio_data_available) = FALSE;
    return wmi_gpio_register_get(ar->arWmi, gpioreg_id);
}

static A_STATUS
ar6000_gpio_intr_ack(struct net_device *dev,
                     A_UINT32 ack_mask)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);

    gpio_intr_available = FALSE;
    return wmi_gpio_intr_ack(ar->arWmi, ack_mask);
}
#endif /* CONFIG_HOST_GPIO_SUPPORT */

#if defined(CONFIG_TARGET_PROFILE_SUPPORT)
static struct prof_count_s prof_count_results;
static A_BOOL prof_count_available; /* Requested GPIO data available */

static A_STATUS
prof_count_get(struct net_device *dev)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);

    *((volatile A_BOOL *)&prof_count_available) = FALSE;
    return wmi_prof_count_get_cmd(ar->arWmi);
}

/*
 * This is called when a response is received from the Target
 * for a previous prof_count_get call.
 */
void
prof_count_rx(A_UINT32 addr, A_UINT32 count)
{
    prof_count_results.addr = addr;
    prof_count_results.count = count;
    *((volatile A_BOOL *)&prof_count_available) = TRUE;
    wake_up(&arEvent);
}
#endif /* CONFIG_TARGET_PROFILE_SUPPORT */

int
ar6000_ioctl_setparam(AR_SOFTC_T *ar, int param, int value)
{
    A_BOOL profChanged = FALSE;
    int ret=0;

    switch (param) {
        case IEEE80211_PARAM_WPA:
            switch (value) {
                case WPA_MODE_WPA1:
                    ar->arAuthMode = WPA_AUTH;
                    profChanged    = TRUE;
                    break;
                case WPA_MODE_WPA2:
                    ar->arAuthMode = WPA2_AUTH;
                    profChanged    = TRUE;
                    break;
                case WPA_MODE_NONE:
                    ar->arAuthMode = NONE_AUTH;
                    profChanged    = TRUE;
                    break;
            }
            break;
        case IEEE80211_PARAM_AUTHMODE:
            switch(value) {
                case IEEE80211_AUTH_WPA_PSK:
                    if (WPA_AUTH == ar->arAuthMode) {
                        ar->arAuthMode = WPA_PSK_AUTH;
                        profChanged    = TRUE;
                    } else if (WPA2_AUTH == ar->arAuthMode) {
                        ar->arAuthMode = WPA2_PSK_AUTH;
                        profChanged    = TRUE;
                    } else {
                        AR_DEBUG_PRINTF("Error -  Setting PSK "\
                            "mode when WPA param was set to %d\n",
                            ar->arAuthMode);
                        ret = -EIO;
                    }
                    break;
                case IEEE80211_AUTH_WPA_CCKM:
                    if (WPA2_AUTH == ar->arAuthMode) {
                        ar->arAuthMode = WPA2_AUTH_CCKM;
                    } else {
                        ar->arAuthMode = WPA_AUTH_CCKM;
                    }
                    break;
                default:
                    break;
            }
            break;
        case IEEE80211_PARAM_UCASTCIPHER:
            switch (value) {
                case IEEE80211_CIPHER_AES_CCM:
                    ar->arPairwiseCrypto = AES_CRYPT;
                    profChanged          = TRUE;
                    break;
                case IEEE80211_CIPHER_TKIP:
                    ar->arPairwiseCrypto = TKIP_CRYPT;
                    profChanged          = TRUE;
                    break;
                case IEEE80211_CIPHER_WEP:
                    ar->arPairwiseCrypto = WEP_CRYPT;
                    profChanged          = TRUE;
                    break;
                case IEEE80211_CIPHER_NONE:
                    ar->arPairwiseCrypto = NONE_CRYPT;
                    profChanged          = TRUE;
                    break;
            }
            break;
        case IEEE80211_PARAM_UCASTKEYLEN:
            if (!IEEE80211_IS_VALID_WEP_CIPHER_LEN(value)) {
                ret = -EIO;
            } else {
                ar->arPairwiseCryptoLen = value;
            }
            break;
        case IEEE80211_PARAM_MCASTCIPHER:
            switch (value) {
                case IEEE80211_CIPHER_AES_CCM:
                    ar->arGroupCrypto = AES_CRYPT;
                    profChanged       = TRUE;
                    break;
                case IEEE80211_CIPHER_TKIP:
                    ar->arGroupCrypto = TKIP_CRYPT;
                    profChanged       = TRUE;
                    break;
                case IEEE80211_CIPHER_WEP:
                    ar->arGroupCrypto = WEP_CRYPT;
                    profChanged       = TRUE;
                    break;
                case IEEE80211_CIPHER_NONE:
                    ar->arGroupCrypto = NONE_CRYPT;
                    profChanged       = TRUE;
                    break;
            }
            break;
        case IEEE80211_PARAM_MCASTKEYLEN:
            if (!IEEE80211_IS_VALID_WEP_CIPHER_LEN(value)) {
                ret = -EIO;
            } else {
                ar->arGroupCryptoLen = value;
            }
            break;
        case IEEE80211_PARAM_COUNTERMEASURES:
            if (ar->arWmiReady == FALSE) {
                return -EIO;
            }
            wmi_set_tkip_countermeasures_cmd(ar->arWmi, value);
            break;
        default:
            break;
    }
    if ((ar->arNextMode != AP_NETWORK) && (profChanged == TRUE)) {
        /*
         * profile has changed.  Erase ssid to signal change
         */
        A_MEMZERO(ar->arSsid, sizeof(ar->arSsid));
    }
    ar->ap_profile_flag = 1; /* There is a change in profile */

    return ret;
}

int
ar6000_ioctl_setkey(AR_SOFTC_T *ar, struct ieee80211req_key *ik)
{
    KEY_USAGE keyUsage;
    A_STATUS status;
    CRYPTO_TYPE keyType = NONE_CRYPT;
    A_UINT8 null_addr[]={0x0,0x0,0x0,0x0,0x0,0x0};
    A_UINT8 bcast_addr[]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

#ifdef USER_KEYS
    ar->user_saved_keys.keyOk = FALSE;
#endif
    if ( (0 == memcmp(ik->ik_macaddr, null_addr, IEEE80211_ADDR_LEN)) ||
         (0 == memcmp(ik->ik_macaddr, bcast_addr, IEEE80211_ADDR_LEN)) ) {
        keyUsage = GROUP_USAGE;
        if(ar->arNextMode == AP_NETWORK) {
            A_MEMCPY(&ar->ap_mode_bkey, ik,
                     sizeof(struct ieee80211req_key));
        }
#ifdef USER_KEYS
        A_MEMCPY(&ar->user_saved_keys.bcast_ik, ik,
                 sizeof(struct ieee80211req_key));
#endif
    } else {
        keyUsage = PAIRWISE_USAGE;
#ifdef USER_KEYS
        A_MEMCPY(&ar->user_saved_keys.ucast_ik, ik,
                 sizeof(struct ieee80211req_key));
#endif
    }

    switch (ik->ik_type) {
        case IEEE80211_CIPHER_WEP:
            keyType = WEP_CRYPT;
            break;
        case IEEE80211_CIPHER_TKIP:
            keyType = TKIP_CRYPT;
            break;
        case IEEE80211_CIPHER_AES_CCM:
            keyType = AES_CRYPT;
            break;
        default:
            break;
    }

    if ((ik->ik_type == IEEE80211_CIPHER_WEP) && (ik->ik_flags & IEEE80211_KEY_XMIT))
    {
        keyUsage |= TX_USAGE;
    }

#ifdef USER_KEYS
    ar->user_saved_keys.keyType = keyType;
#endif
    if (IEEE80211_CIPHER_CCKM_KRK != ik->ik_type) {
        if (NONE_CRYPT == keyType) {
            return -EIO;
        }

        if (((WPA_PSK_AUTH == ar->arAuthMode) || (WPA2_PSK_AUTH == ar->arAuthMode)) &&
            (GROUP_USAGE == keyUsage))
        {
            A_UNTIMEOUT(&ar->disconnect_timer);
        }

        status = wmi_addKey_cmd(ar->arWmi, ik->ik_keyix, keyType, keyUsage,
                                ik->ik_keylen, (A_UINT8 *)&ik->ik_keyrsc,
                                ik->ik_keydata, KEY_OP_INIT_VAL, ik->ik_macaddr,
                                SYNC_BOTH_WMIFLAG);

        if (status != A_OK) {
            return -EIO;
        }
    } else {
        status = wmi_add_krk_cmd(ar->arWmi, ik->ik_keydata);
    }

#ifdef USER_KEYS
    ar->user_saved_keys.keyOk = TRUE;
#endif

    return 0;
}

int ar6000_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    HIF_DEVICE *hifDevice = ar->arHifDevice;
    int ret, param;
    unsigned int address = 0;
    unsigned int length = 0;
    unsigned char *buffer;
    char *userdata;
    A_UINT32 connectCtrlFlags;

    WMI_SET_AKMP_PARAMS_CMD  akmpParams;
    WMI_SET_PMKID_LIST_CMD   pmkidInfo;

    if (cmd == AR6000_IOCTL_EXTENDED) {
        /*
         * This allows for many more wireless ioctls than would otherwise
         * be available.  Applications embed the actual ioctl command in
         * the first word of the parameter block, and use the command
         * AR6000_IOCTL_EXTENDED_CMD on the ioctl call.
         */
        get_user(cmd, (int *)rq->ifr_data);
        userdata = (char *)(((unsigned int *)rq->ifr_data)+1);

        if(is_xioctl_allowed(ar->arNextMode, cmd) != A_OK) {
            A_PRINTF("xioctl: cmd=%d not allowed in this mode\n",cmd);
            return -EOPNOTSUPP;
        }
    } else {
        A_STATUS ret = is_iwioctl_allowed(ar->arNextMode, cmd);
        if(ret == A_ENOTSUP) {
            A_PRINTF("iwioctl: cmd=0x%x not allowed in this mode\n", cmd);
            return -EOPNOTSUPP;
        } else if (ret == A_ERROR) {
            /* It is not our ioctl (out of range ioctl) */
            return -EOPNOTSUPP;
        }
        userdata = (char *)rq->ifr_data;
    }

    if ((ar->arWlanState == WLAN_DISABLED) &&
        ((cmd != AR6000_XIOCTRL_WMI_SET_WLAN_STATE) &&
         (cmd != AR6000_XIOCTL_DIAG_READ) &&
         (cmd != AR6000_XIOCTL_DIAG_WRITE)))
    {
        return -EIO;
    }

    ret = 0;
    switch(cmd)
    {
        case IEEE80211_IOCTL_SETPARAM:
        {
            int param, value;
            int *ptr = (int *)rq->ifr_ifru.ifru_newname;
            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else {
                param = *ptr++;
                value = *ptr;
                ret = ar6000_ioctl_setparam(ar,param,value);
            }
            break;
        }
        case IEEE80211_IOCTL_SETKEY:
        {
            struct ieee80211req_key keydata;
            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&keydata, userdata,
                            sizeof(struct ieee80211req_key))) {
                ret = -EFAULT;
            } else {
                ar6000_ioctl_setkey(ar, &keydata);
            }
            break;
        }
        case IEEE80211_IOCTL_DELKEY:
        case IEEE80211_IOCTL_SETOPTIE:
        {
            /*ret = -EIO; */
            break;
        }
        case IEEE80211_IOCTL_SETMLME:
        {
            struct ieee80211req_mlme mlme;
            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&mlme, userdata,
                            sizeof(struct ieee80211req_mlme))) {
                ret = -EFAULT;
            } else {
                switch (mlme.im_op) {
                    case IEEE80211_MLME_AUTHORIZE:
                        A_PRINTF("setmlme AUTHORIZE %02X:%02X\n",
                            mlme.im_macaddr[4], mlme.im_macaddr[5]);
                        break;
                    case IEEE80211_MLME_UNAUTHORIZE:
                        A_PRINTF("setmlme UNAUTHORIZE %02X:%02X\n",
                            mlme.im_macaddr[4], mlme.im_macaddr[5]);
                        break;
                    case IEEE80211_MLME_DEAUTH:
                        A_PRINTF("setmlme DEAUTH %02X:%02X\n",
                            mlme.im_macaddr[4], mlme.im_macaddr[5]);
                        /*remove_sta(ar, mlme.im_macaddr); */
                        break;
                    case IEEE80211_MLME_DISASSOC:
                        A_PRINTF("setmlme DISASSOC %02X:%02X\n",
                            mlme.im_macaddr[4], mlme.im_macaddr[5]);
                        /*remove_sta(ar, mlme.im_macaddr); */
                        break;
                    default:
                        return 0;
                }

                wmi_ap_set_mlme(ar->arWmi, mlme.im_op, mlme.im_macaddr,
                                mlme.im_reason);
            }
            break;
        }
        case IEEE80211_IOCTL_ADDPMKID:
        {
            struct ieee80211req_addpmkid  req;
            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&req, userdata, sizeof(struct ieee80211req_addpmkid))) {
                ret = -EFAULT;
            } else {
                A_STATUS status;

                AR_DEBUG_PRINTF("Add pmkid for %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x en=%d\n",
                    req.pi_bssid[0], req.pi_bssid[1], req.pi_bssid[2],
                    req.pi_bssid[3], req.pi_bssid[4], req.pi_bssid[5],
                    req.pi_enable);

                status = wmi_setPmkid_cmd(ar->arWmi, req.pi_bssid, req.pi_pmkid,
                              req.pi_enable);

                if (status != A_OK) {
                    return -EIO;
                }
            }
            break;
        }
#ifdef CONFIG_HOST_TCMD_SUPPORT
        case AR6000_XIOCTL_TCMD_CONT_TX:
            {
                TCMD_CONT_TX txCmd;

                if (ar->tcmdPm == TCMD_PM_SLEEP) {
                    A_PRINTF("Can NOT send tx tcmd when target is asleep! \n");
                    return -EFAULT;
                }

                if(copy_from_user(&txCmd, userdata, sizeof(TCMD_CONT_TX)))
                    return -EFAULT;
                wmi_test_cmd(ar->arWmi,(A_UINT8 *)&txCmd, sizeof(TCMD_CONT_TX));
            }
            break;
        case AR6000_XIOCTL_TCMD_CONT_RX:
            {
                TCMD_CONT_RX rxCmd;

                if (ar->tcmdPm == TCMD_PM_SLEEP) {
                    A_PRINTF("Can NOT send rx tcmd when target is asleep! \n");
                    return -EFAULT;
                }
                if(copy_from_user(&rxCmd, userdata, sizeof(TCMD_CONT_RX)))
                    return -EFAULT;
                switch(rxCmd.act)
                {
                    case TCMD_CONT_RX_PROMIS:
                    case TCMD_CONT_RX_FILTER:
                    case TCMD_CONT_RX_SETMAC:
                    case TCMD_CONT_RX_SET_ANT_SWITCH_TABLE:
                         wmi_test_cmd(ar->arWmi,(A_UINT8 *)&rxCmd,
                                                sizeof(TCMD_CONT_RX));
                         break;
                    case TCMD_CONT_RX_REPORT:
                         ar6000_ioctl_tcmd_get_rx_report(dev, rq,
                         (A_UINT8 *)&rxCmd, sizeof(TCMD_CONT_RX));
                         break;
                    default:
                         A_PRINTF("Unknown Cont Rx mode: %d\n",rxCmd.act);
                         return -EINVAL;
                }
            }
            break;
        case AR6000_XIOCTL_TCMD_PM:
            {
                TCMD_PM pmCmd;

                if(copy_from_user(&pmCmd, userdata, sizeof(TCMD_PM)))
                    return -EFAULT;
                ar->tcmdPm = pmCmd.mode;
                wmi_test_cmd(ar->arWmi, (A_UINT8*)&pmCmd, sizeof(TCMD_PM));
            }
            break;
        case AR6000_XIOCTL_TCMD_GET_MAC:
            {
                TCMD_GET_MAC getMacCmd;

                ar6000_ioctl_tcmd_get_mac(dev, rq,
                    (A_UINT8 *)&getMacCmd, sizeof(TCMD_GET_MAC));
            }
            break;
#endif /* CONFIG_HOST_TCMD_SUPPORT */

        case AR6000_XIOCTL_BMI_DONE:
            if(bmienable)
            {
                ret = ar6000_init(dev);
            }
            else
            {
                ret = BMIDone(hifDevice);
            }
            break;

        case AR6000_XIOCTL_BMI_READ_MEMORY:
            get_user(address, (unsigned int *)userdata);
            get_user(length, (unsigned int *)userdata + 1);
            AR_DEBUG_PRINTF("Read Memory (address: 0x%x, length: %d)\n",
                             address, length);
            if ((buffer = (unsigned char *)A_MALLOC(length)) != NULL) {
                A_MEMZERO(buffer, length);
                ret = BMIReadMemory(hifDevice, address, buffer, length);
                if (copy_to_user(rq->ifr_data, buffer, length)) {
                    ret = -EFAULT;
                }
                A_FREE(buffer);
            } else {
                ret = -ENOMEM;
            }
            break;

        case AR6000_XIOCTL_BMI_WRITE_MEMORY:
            get_user(address, (unsigned int *)userdata);
            get_user(length, (unsigned int *)userdata + 1);
            AR_DEBUG_PRINTF("Write Memory (address: 0x%x, length: %d)\n",
                             address, length);
            if ((buffer = (unsigned char *)A_MALLOC(length)) != NULL) {
                A_MEMZERO(buffer, length);
                if (copy_from_user(buffer, &userdata[sizeof(address) +
                                   sizeof(length)], length))
                {
                    ret = -EFAULT;
                } else {
                    ret = BMIWriteMemory(hifDevice, address, buffer, length);
                }
                A_FREE(buffer);
            } else {
                ret = -ENOMEM;
            }
            break;

        case AR6000_XIOCTL_BMI_TEST:
/* ATHENV */
#ifdef ANDROID_ENV
            ret = A_OK;
            put_user( wait_event_interruptible_timeout(
                                          arEvent,
                                          (ar->arWmiReady == TRUE),
                                          wmitimeout * HZ),
                      (unsigned int *)rq->ifr_data );
            if (ar->arWmiReady == TRUE)
                put_user(1, (unsigned int *)rq->ifr_data);
            break;
#else
           AR_DEBUG_PRINTF("No longer supported\n");
           ret = -EOPNOTSUPP;
           break;
#endif
/* ATHENV */

        case AR6000_XIOCTL_BMI_EXECUTE:
            get_user(address, (unsigned int *)userdata);
            get_user(param, (unsigned int *)userdata + 1);
            AR_DEBUG_PRINTF("Execute (address: 0x%x, param: %d)\n",
                             address, param);
            ret = BMIExecute(hifDevice, address, &param);
            put_user(param, (unsigned int *)rq->ifr_data); /* return value */
            break;

        case AR6000_XIOCTL_BMI_SET_APP_START:
            get_user(address, (unsigned int *)userdata);
            AR_DEBUG_PRINTF("Set App Start (address: 0x%x)\n", address);
            ret = BMISetAppStart(hifDevice, address);
            break;

        case AR6000_XIOCTL_BMI_READ_SOC_REGISTER:
            get_user(address, (unsigned int *)userdata);
            ret = BMIReadSOCRegister(hifDevice, address, &param);
            put_user(param, (unsigned int *)rq->ifr_data); /* return value */
            break;

        case AR6000_XIOCTL_BMI_WRITE_SOC_REGISTER:
            get_user(address, (unsigned int *)userdata);
            get_user(param, (unsigned int *)userdata + 1);
            ret = BMIWriteSOCRegister(hifDevice, address, param);
            break;

#ifdef HTC_RAW_INTERFACE
        case AR6000_XIOCTL_HTC_RAW_OPEN:
            ret = A_OK;
            if (!arRawIfEnabled(ar)) {
                /* make sure block size is set in case the target was reset since last
                  * BMI phase (i.e. flashup downloads) */
                ret = ar6000_set_htc_params(ar->arHifDevice,
                                            ar->arTargetType,
                                            0,  /* use default yield */
                                            0   /* use default number of HTC ctrl buffers */
                                            );
                if (A_FAILED(ret)) {
                    break;
                }
                /* Terminate the BMI phase */
                ret = BMIDone(hifDevice);
                if (ret == A_OK) {
                    ret = ar6000_htc_raw_open(ar);
                }
            }
            break;

        case AR6000_XIOCTL_HTC_RAW_CLOSE:
            if (arRawIfEnabled(ar)) {
                ret = ar6000_htc_raw_close(ar);
                arRawIfEnabled(ar) = FALSE;
            } else {
                ret = A_ERROR;
            }
            break;

        case AR6000_XIOCTL_HTC_RAW_READ:
            if (arRawIfEnabled(ar)) {
                unsigned int streamID;
                get_user(streamID, (unsigned int *)userdata);
                get_user(length, (unsigned int *)userdata + 1);
                buffer = rq->ifr_data + sizeof(length);
                ret = ar6000_htc_raw_read(ar, (HTC_RAW_STREAM_ID)streamID,
                                          buffer, length);
                /* ATHENV V7.2 +++ */
                /* Without this, ART application can not run well on 8K Android */
                /*printk("AR6K: length=%d, ret=%d\n", length, ret); */
                if (ret != length)
                    printk("[HTC_RAW_READ] Read %d bytes, return %d bytes\n", length, ret);
                /* ATHENV V7.2 --- */
                put_user(ret, (unsigned int *)rq->ifr_data);
            } else {
                ret = A_ERROR;
            }
            break;

        case AR6000_XIOCTL_HTC_RAW_WRITE:
            if (arRawIfEnabled(ar)) {
                unsigned int streamID;
                get_user(streamID, (unsigned int *)userdata);
                get_user(length, (unsigned int *)userdata + 1);
                buffer = userdata + sizeof(streamID) + sizeof(length);
                ret = ar6000_htc_raw_write(ar, (HTC_RAW_STREAM_ID)streamID,
                                           buffer, length);
                /* ATHENV V7.2 +++ */
                /* Without this, ART application can not run well on 8K Android */
                /*printk("AR6K: length=%d, ret=%d\n", length, ret); */
                if (ret != length)
                    printk("[HTC_RAW_WRITE] Write %d bytes, return %d bytes\n", length, ret);
                /* ATHENV V7.2 --- */
                put_user(ret, (unsigned int *)rq->ifr_data);
            } else {
                ret = A_ERROR;
            }
            break;
#endif /* HTC_RAW_INTERFACE */

        case AR6000_XIOCTL_BMI_LZ_STREAM_START:
            get_user(address, (unsigned int *)userdata);
            AR_DEBUG_PRINTF("Start Compressed Stream (address: 0x%x)\n", address);
            ret = BMILZStreamStart(hifDevice, address);
            break;

        case AR6000_XIOCTL_BMI_LZ_DATA:
            get_user(length, (unsigned int *)userdata);
            AR_DEBUG_PRINTF("Send Compressed Data (length: %d)\n", length);
            if ((buffer = (unsigned char *)A_MALLOC(length)) != NULL) {
                A_MEMZERO(buffer, length);
                if (copy_from_user(buffer, &userdata[sizeof(length)], length))
                {
                    ret = -EFAULT;
                } else {
                    ret = BMILZData(hifDevice, buffer, length);
                }
                A_FREE(buffer);
            } else {
                ret = -ENOMEM;
            }
            break;

#if defined(CONFIG_TARGET_PROFILE_SUPPORT)
        /*
         * Optional support for Target-side profiling.
         * Not needed in production.
         */

        /* Configure Target-side profiling */
        case AR6000_XIOCTL_PROF_CFG:
        {
            A_UINT32 period;
            A_UINT32 nbins;
            get_user(period, (unsigned int *)userdata);
            get_user(nbins, (unsigned int *)userdata + 1);

            if (wmi_prof_cfg_cmd(ar->arWmi, period, nbins) != A_OK) {
                ret = -EIO;
            }

            break;
        }

        /* Start a profiling bucket/bin at the specified address */
        case AR6000_XIOCTL_PROF_ADDR_SET:
        {
            A_UINT32 addr;
            get_user(addr, (unsigned int *)userdata);

            if (wmi_prof_addr_set_cmd(ar->arWmi, addr) != A_OK) {
                ret = -EIO;
            }

            break;
        }

        /* START Target-side profiling */
        case AR6000_XIOCTL_PROF_START:
            wmi_prof_start_cmd(ar->arWmi);
            break;

        /* STOP Target-side profiling */
        case AR6000_XIOCTL_PROF_STOP:
            wmi_prof_stop_cmd(ar->arWmi);
            break;
        case AR6000_XIOCTL_PROF_COUNT_GET:
        {
            if (ar->bIsDestroyProgress) {
                return -EBUSY;
            }
            if (ar->arWmiReady == FALSE) {
                return -EIO;
            }
            if (down_interruptible(&ar->arSem)) {
                return -ERESTARTSYS;
            }
            if (ar->bIsDestroyProgress) {
                up(&ar->arSem);
                return -EBUSY;
            }

            prof_count_available = FALSE;
            ret = prof_count_get(dev);
            if (ret != A_OK) {
                up(&ar->arSem);
                return -EIO;
            }

            /* Wait for Target to respond. */
            wait_event_interruptible(arEvent, prof_count_available);
            if (signal_pending(current)) {
                ret = -EINTR;
            } else {
                if (copy_to_user(userdata, &prof_count_results,
                                 sizeof(prof_count_results)))
                {
                    ret = -EFAULT;
                }
            }
            up(&ar->arSem);
            break;
        }
#endif /* CONFIG_TARGET_PROFILE_SUPPORT */

        case AR6000_IOCTL_WMI_GETREV:
        {
            if (copy_to_user(rq->ifr_data, &ar->arVersion,
                             sizeof(ar->arVersion)))
            {
                ret = -EFAULT;
            }
            break;
        }
        case AR6000_IOCTL_WMI_SETPWR:
        {
            WMI_POWER_MODE_CMD pwrModeCmd;

            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&pwrModeCmd, userdata,
                                   sizeof(pwrModeCmd)))
            {
                ret = -EFAULT;
            } else {
                if (wmi_powermode_cmd(ar->arWmi, pwrModeCmd.powerMode)
                       != A_OK)
                {
                    ret = -EIO;
                }
            }
            break;
        }
        case AR6000_IOCTL_WMI_SET_IBSS_PM_CAPS:
        {
            WMI_IBSS_PM_CAPS_CMD ibssPmCaps;

            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&ibssPmCaps, userdata,
                                   sizeof(ibssPmCaps)))
            {
                ret = -EFAULT;
            } else {
                if (wmi_ibsspmcaps_cmd(ar->arWmi, ibssPmCaps.power_saving, ibssPmCaps.ttl,
                    ibssPmCaps.atim_windows, ibssPmCaps.timeout_value) != A_OK)
                {
                    ret = -EIO;
                }
                AR6000_SPIN_LOCK(&ar->arLock, 0);
                ar->arIbssPsEnable = ibssPmCaps.power_saving;
                AR6000_SPIN_UNLOCK(&ar->arLock, 0);
            }
            break;
        }
        case AR6000_IOCTL_WMI_SET_PMPARAMS:
        {
            WMI_POWER_PARAMS_CMD pmParams;

            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&pmParams, userdata,
                                      sizeof(pmParams)))
            {
                ret = -EFAULT;
            } else {
                if (wmi_pmparams_cmd(ar->arWmi, pmParams.idle_period,
                                     pmParams.pspoll_number,
                                     pmParams.dtim_policy) != A_OK)
                {
                    ret = -EIO;
                }
            }
            break;
        }
        case AR6000_IOCTL_WMI_SETSCAN:
        {
            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&ar->scParams, userdata,
                                      sizeof(ar->scParams)))
            {
                ret = -EFAULT;
            } else {
                if (CAN_SCAN_IN_CONNECT(ar->scParams.scanCtrlFlags)) {
                    ar->arSkipScan = FALSE;
                } else {
                    ar->arSkipScan = TRUE;
                }

                if (wmi_scanparams_cmd(ar->arWmi, ar->scParams.fg_start_period,
                                       ar->scParams.fg_end_period,
                                       ar->scParams.bg_period,
                                       ar->scParams.minact_chdwell_time,
                                       ar->scParams.maxact_chdwell_time,
                                       ar->scParams.pas_chdwell_time,
                                       ar->scParams.shortScanRatio,
                                       ar->scParams.scanCtrlFlags,
                                       ar->scParams.max_dfsch_act_time,
                                       ar->scParams.maxact_scan_per_ssid) != A_OK)
                {
                    ret = -EIO;
                }
            }
            break;
        }
        case AR6000_IOCTL_WMI_SETLISTENINT:
        {
            WMI_LISTEN_INT_CMD listenCmd;

            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&listenCmd, userdata,
                                      sizeof(listenCmd)))
            {
                ret = -EFAULT;
            } else {
                    if (wmi_listeninterval_cmd(ar->arWmi, listenCmd.listenInterval, listenCmd.numBeacons) != A_OK) {
                        ret = -EIO;
                    } else {
                        AR6000_SPIN_LOCK(&ar->arLock, 0);
                        ar->arListenInterval = param;
                        AR6000_SPIN_UNLOCK(&ar->arLock, 0);
                    }

                }
            break;
        }
        case AR6000_IOCTL_WMI_SET_BMISS_TIME:
        {
            WMI_BMISS_TIME_CMD bmissCmd;

            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&bmissCmd, userdata,
                                      sizeof(bmissCmd)))
            {
                ret = -EFAULT;
            } else {
                if (wmi_bmisstime_cmd(ar->arWmi, bmissCmd.bmissTime, bmissCmd.numBeacons) != A_OK) {
                    ret = -EIO;
                }
            }
            break;
        }
        case AR6000_IOCTL_WMI_SETBSSFILTER:
        {
            WMI_BSS_FILTER_CMD filt;

            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&filt, userdata,
                                   sizeof(filt)))
            {
                ret = -EFAULT;
            } else {
                if (wmi_bssfilter_cmd(ar->arWmi, filt.bssFilter, filt.ieMask)
                        != A_OK) {
                    ret = -EIO;
                }
            }
            break;
        }
        case AR6000_IOCTL_WMI_SET_SNRTHRESHOLD:
        {
            ret = ar6000_ioctl_set_snr_threshold(dev, rq);
            break;
        }
        case AR6000_XIOCTL_WMI_SET_RSSITHRESHOLD:
        {
            ret = ar6000_ioctl_set_rssi_threshold(dev, rq);
            break;
        }
        case AR6000_XIOCTL_WMI_CLR_RSSISNR:
        {
            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            }
            ret = wmi_clr_rssi_snr(ar->arWmi);
            break;
        }
        case AR6000_XIOCTL_WMI_SET_LQTHRESHOLD:
        {
            ret = ar6000_ioctl_set_lq_threshold(dev, rq);
            break;
        }
        case AR6000_XIOCTL_WMI_SET_LPREAMBLE:
        {
            WMI_SET_LPREAMBLE_CMD setLpreambleCmd;

            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&setLpreambleCmd, userdata,
                                   sizeof(setLpreambleCmd)))
            {
                ret = -EFAULT;
            } else {
                if (wmi_set_lpreamble_cmd(ar->arWmi, setLpreambleCmd.status)
                       != A_OK)
                {
                    ret = -EIO;
                }
            }

            break;
        }
        case AR6000_XIOCTL_WMI_SET_RTS:
        {
            WMI_SET_RTS_CMD rtsCmd;

            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&rtsCmd, userdata,
                                   sizeof(rtsCmd)))
            {
                ret = -EFAULT;
            } else {
                ar->arRTS = rtsCmd.threshold;
                if (wmi_set_rts_cmd(ar->arWmi, rtsCmd.threshold)
                       != A_OK)
                {
                    ret = -EIO;
                }
            }

            break;
        }
        case AR6000_XIOCTL_WMI_SET_WMM:
        {
            ret = ar6000_ioctl_set_wmm(dev, rq);
            break;
        }
        case AR6000_XIOCTL_WMI_SET_TXOP:
        {
            ret = ar6000_ioctl_set_txop(dev, rq);
            break;
        }
        case AR6000_XIOCTL_WMI_GET_RD:
        {
            ret = ar6000_ioctl_get_rd(dev, rq);
            break;
        }
        case AR6000_IOCTL_WMI_SET_CHANNELPARAMS:
        {
            ret = ar6000_ioctl_set_channelParams(dev, rq);
            break;
        }
        case AR6000_IOCTL_WMI_SET_PROBEDSSID:
        {
            ret = ar6000_ioctl_set_probedSsid(dev, rq);
            break;
        }
        case AR6000_IOCTL_WMI_SET_BADAP:
        {
            ret = ar6000_ioctl_set_badAp(dev, rq);
            break;
        }
        case AR6000_IOCTL_WMI_CREATE_QOS:
        {
            ret = ar6000_ioctl_create_qos(dev, rq);
            break;
        }
        case AR6000_IOCTL_WMI_DELETE_QOS:
        {
            ret = ar6000_ioctl_delete_qos(dev, rq);
            break;
        }
        case AR6000_IOCTL_WMI_GET_QOS_QUEUE:
        {
            ret = ar6000_ioctl_get_qos_queue(dev, rq);
            break;
        }
        case AR6000_IOCTL_WMI_GET_TARGET_STATS:
        {
            ret = ar6000_ioctl_get_target_stats(dev, rq);
            break;
        }
        case AR6000_IOCTL_WMI_SET_ERROR_REPORT_BITMASK:
        {
            ret = ar6000_ioctl_set_error_report_bitmask(dev, rq);
            break;
        }
        case AR6000_IOCTL_WMI_SET_ASSOC_INFO:
        {
            WMI_SET_ASSOC_INFO_CMD cmd;
            A_UINT8 assocInfo[WMI_MAX_ASSOC_INFO_LEN];

            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else {
                get_user(cmd.ieType, userdata);
                if (cmd.ieType >= WMI_MAX_ASSOC_INFO_TYPE) {
                    ret = -EIO;
                } else {
                    get_user(cmd.bufferSize, userdata + 1);
                    if (cmd.bufferSize > WMI_MAX_ASSOC_INFO_LEN) {
                        ret = -EFAULT;
                        break;
                    }
                    if (copy_from_user(assocInfo, userdata + 2,
                                       cmd.bufferSize))
                    {
                        ret = -EFAULT;
                    } else {
                        if (wmi_associnfo_cmd(ar->arWmi, cmd.ieType,
                                                 cmd.bufferSize,
                                                 assocInfo) != A_OK)
                        {
                            ret = -EIO;
                        }
                    }
                }
            }
            break;
        }
        case AR6000_IOCTL_WMI_SET_ACCESS_PARAMS:
        {
            ret = ar6000_ioctl_set_access_params(dev, rq);
            break;
        }
        case AR6000_IOCTL_WMI_SET_DISC_TIMEOUT:
        {
            ret = ar6000_ioctl_set_disconnect_timeout(dev, rq);
            break;
        }
        case AR6000_XIOCTL_FORCE_TARGET_RESET:
        {
            if (ar->arHtcTarget)
            {
/*                HTCForceReset(htcTarget); */
            }
            else
            {
                AR_DEBUG_PRINTF("ar6000_ioctl cannot attempt reset.\n");
            }
            break;
        }
        case AR6000_XIOCTL_TARGET_INFO:
        case AR6000_XIOCTL_CHECK_TARGET_READY: /* backwards compatibility */
        {
            /* If we made it to here, then the Target exists and is ready. */

            if (cmd == AR6000_XIOCTL_TARGET_INFO) {
                if (copy_to_user((A_UINT32 *)rq->ifr_data, &ar->arVersion.target_ver,
                                 sizeof(ar->arVersion.target_ver)))
                {
                    ret = -EFAULT;
                }
                if (copy_to_user(((A_UINT32 *)rq->ifr_data)+1, &ar->arTargetType,
                                 sizeof(ar->arTargetType)))
                {
                    ret = -EFAULT;
                }
            }
            break;
        }
        case AR6000_XIOCTL_WMI_SET_HB_CHALLENGE_RESP_PARAMS:
        {
            WMI_SET_HB_CHALLENGE_RESP_PARAMS_CMD hbparam;

            if (copy_from_user(&hbparam, userdata, sizeof(hbparam)))
            {
                ret = -EFAULT;
            } else {
                AR6000_SPIN_LOCK(&ar->arLock, 0);
                /* Start a cyclic timer with the parameters provided. */
                if (hbparam.frequency) {
                    ar->arHBChallengeResp.frequency = hbparam.frequency;
                }
                if (hbparam.threshold) {
                    ar->arHBChallengeResp.missThres = hbparam.threshold;
                }

                /* Delete the pending timer and start a new one */
                if (timer_pending(&ar->arHBChallengeResp.timer)) {
                    A_UNTIMEOUT(&ar->arHBChallengeResp.timer);
                }
                A_TIMEOUT_MS(&ar->arHBChallengeResp.timer, ar->arHBChallengeResp.frequency * 1000, 0);
                AR6000_SPIN_UNLOCK(&ar->arLock, 0);
            }
            break;
        }
        case AR6000_XIOCTL_WMI_GET_HB_CHALLENGE_RESP:
        {
            A_UINT32 cookie;

            if (copy_from_user(&cookie, userdata, sizeof(cookie))) {
                return -EFAULT;
            }

            /* Send the challenge on the control channel */
            if (wmi_get_challenge_resp_cmd(ar->arWmi, cookie, APP_HB_CHALLENGE) != A_OK) {
                return -EIO;
            }
            break;
        }
#ifdef USER_KEYS
        case AR6000_XIOCTL_USER_SETKEYS:
        {

            ar->user_savedkeys_stat = USER_SAVEDKEYS_STAT_RUN;

            if (copy_from_user(&ar->user_key_ctrl, userdata,
                               sizeof(ar->user_key_ctrl)))
            {
                return -EFAULT;
            }

            A_PRINTF("ar6000 USER set key %x\n", ar->user_key_ctrl);
            break;
        }
#endif /* USER_KEYS */

#ifdef CONFIG_HOST_GPIO_SUPPORT
        case AR6000_XIOCTL_GPIO_OUTPUT_SET:
        {
            struct ar6000_gpio_output_set_cmd_s gpio_output_set_cmd;

            if (ar->bIsDestroyProgress) {
                return -EBUSY;
            }
            if (ar->arWmiReady == FALSE) {
                return -EIO;
            }
            if (down_interruptible(&ar->arSem)) {
                return -ERESTARTSYS;
            }
            if (ar->bIsDestroyProgress) {
                up(&ar->arSem);
                return -EBUSY;
            }

            if (copy_from_user(&gpio_output_set_cmd, userdata,
                                sizeof(gpio_output_set_cmd)))
            {
                ret = -EFAULT;
            } else {
                ret = ar6000_gpio_output_set(dev,
                                             gpio_output_set_cmd.set_mask,
                                             gpio_output_set_cmd.clear_mask,
                                             gpio_output_set_cmd.enable_mask,
                                             gpio_output_set_cmd.disable_mask);
                if (ret != A_OK) {
                    ret = EIO;
                }
            }
            up(&ar->arSem);
            break;
        }
        case AR6000_XIOCTL_GPIO_INPUT_GET:
        {
            if (ar->bIsDestroyProgress) {
                return -EBUSY;
            }
            if (ar->arWmiReady == FALSE) {
                return -EIO;
            }
            if (down_interruptible(&ar->arSem)) {
                return -ERESTARTSYS;
            }
            if (ar->bIsDestroyProgress) {
                up(&ar->arSem);
                return -EBUSY;
            }

            ret = ar6000_gpio_input_get(dev);
            if (ret != A_OK) {
                up(&ar->arSem);
                return -EIO;
            }

            /* Wait for Target to respond. */
            wait_event_interruptible(arEvent, gpio_data_available);
            if (signal_pending(current)) {
                ret = -EINTR;
            } else {
                A_ASSERT(gpio_reg_results.gpioreg_id == GPIO_ID_NONE);

                if (copy_to_user(userdata, &gpio_reg_results.value,
                                 sizeof(gpio_reg_results.value)))
                {
                    ret = -EFAULT;
                }
            }
            up(&ar->arSem);
            break;
        }
        case AR6000_XIOCTL_GPIO_REGISTER_SET:
        {
            struct ar6000_gpio_register_cmd_s gpio_register_cmd;

            if (ar->bIsDestroyProgress) {
                return -EBUSY;
            }
            if (ar->arWmiReady == FALSE) {
                return -EIO;
            }
            if (down_interruptible(&ar->arSem)) {
                return -ERESTARTSYS;
            }
            if (ar->bIsDestroyProgress) {
                up(&ar->arSem);
                return -EBUSY;
            }

            if (copy_from_user(&gpio_register_cmd, userdata,
                                sizeof(gpio_register_cmd)))
            {
                ret = -EFAULT;
            } else {
                ret = ar6000_gpio_register_set(dev,
                                               gpio_register_cmd.gpioreg_id,
                                               gpio_register_cmd.value);
                if (ret != A_OK) {
                    ret = EIO;
                }

                /* Wait for acknowledgement from Target */
                wait_event_interruptible(arEvent, gpio_ack_received);
                if (signal_pending(current)) {
                    ret = -EINTR;
                }
            }
            up(&ar->arSem);
            break;
        }
        case AR6000_XIOCTL_GPIO_REGISTER_GET:
        {
            struct ar6000_gpio_register_cmd_s gpio_register_cmd;

            if (ar->bIsDestroyProgress) {
                return -EBUSY;
            }
            if (ar->arWmiReady == FALSE) {
                return -EIO;
            }
            if (down_interruptible(&ar->arSem)) {
                return -ERESTARTSYS;
            }
            if (ar->bIsDestroyProgress) {
                up(&ar->arSem);
                return -EBUSY;
            }

            if (copy_from_user(&gpio_register_cmd, userdata,
                                sizeof(gpio_register_cmd)))
            {
                ret = -EFAULT;
            } else {
                ret = ar6000_gpio_register_get(dev, gpio_register_cmd.gpioreg_id);
                if (ret != A_OK) {
                    up(&ar->arSem);
                    return -EIO;
                }

                /* Wait for Target to respond. */
                wait_event_interruptible(arEvent, gpio_data_available);
                if (signal_pending(current)) {
                    ret = -EINTR;
                } else {
                    A_ASSERT(gpio_register_cmd.gpioreg_id == gpio_reg_results.gpioreg_id);
                    if (copy_to_user(userdata, &gpio_reg_results,
                                     sizeof(gpio_reg_results)))
                    {
                        ret = -EFAULT;
                    }
                }
            }
            up(&ar->arSem);
            break;
        }
        case AR6000_XIOCTL_GPIO_INTR_ACK:
        {
            struct ar6000_gpio_intr_ack_cmd_s gpio_intr_ack_cmd;

            if (ar->bIsDestroyProgress) {
                return -EBUSY;
            }
            if (ar->arWmiReady == FALSE) {
                return -EIO;
            }
            if (down_interruptible(&ar->arSem)) {
                return -ERESTARTSYS;
            }
            if (ar->bIsDestroyProgress) {
                up(&ar->arSem);
                return -EBUSY;
            }

            if (copy_from_user(&gpio_intr_ack_cmd, userdata,
                                sizeof(gpio_intr_ack_cmd)))
            {
                ret = -EFAULT;
            } else {
                ret = ar6000_gpio_intr_ack(dev, gpio_intr_ack_cmd.ack_mask);
                if (ret != A_OK) {
                    ret = EIO;
                }
            }
            up(&ar->arSem);
            break;
        }
        case AR6000_XIOCTL_GPIO_INTR_WAIT:
        {
            /* Wait for Target to report an interrupt. */
            dev_hold(dev);
            rtnl_unlock();
            wait_event_interruptible(arEvent, gpio_intr_available);
            rtnl_lock();
            __dev_put(dev);

            if (signal_pending(current)) {
                ret = -EINTR;
            } else {
                if (copy_to_user(userdata, &gpio_intr_results,
                                 sizeof(gpio_intr_results)))
                {
                    ret = -EFAULT;
                }
            }
            break;
        }
#endif /* CONFIG_HOST_GPIO_SUPPORT */

        case AR6000_XIOCTL_DBGLOG_CFG_MODULE:
        {
            struct ar6000_dbglog_module_config_s config;

            if (copy_from_user(&config, userdata, sizeof(config))) {
                return -EFAULT;
            }

            /* Send the challenge on the control channel */
            if (wmi_config_debug_module_cmd(ar->arWmi, config.mmask,
                                            config.tsr, config.rep,
                                            config.size, config.valid) != A_OK)
            {
                return -EIO;
            }
            break;
        }

        case AR6000_XIOCTL_DBGLOG_GET_DEBUG_LOGS:
        {
            /* Send the challenge on the control channel */
            if (ar6000_dbglog_get_debug_logs(ar) != A_OK)
            {
                return -EIO;
            }
            break;
        }

        case AR6000_XIOCTL_SET_ADHOC_BSSID:
        {
            WMI_SET_ADHOC_BSSID_CMD adhocBssid;

            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&adhocBssid, userdata,
                                      sizeof(adhocBssid)))
            {
                ret = -EFAULT;
            } else if (A_MEMCMP(adhocBssid.bssid, bcast_mac,
                                AR6000_ETH_ADDR_LEN) == 0)
            {
                ret = -EFAULT;
            } else {

                A_MEMCPY(ar->arReqBssid, adhocBssid.bssid, sizeof(ar->arReqBssid));
        }
            break;
        }

        case AR6000_XIOCTL_SET_OPT_MODE:
        {
        WMI_SET_OPT_MODE_CMD optModeCmd;
            AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);

            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&optModeCmd, userdata,
                                      sizeof(optModeCmd)))
            {
                ret = -EFAULT;
            } else if (ar->arConnected && optModeCmd.optMode == SPECIAL_ON) {
                ret = -EFAULT;

            } else if (wmi_set_opt_mode_cmd(ar->arWmi, optModeCmd.optMode)
                       != A_OK)
            {
                ret = -EIO;
            }
            break;
        }

        case AR6000_XIOCTL_OPT_SEND_FRAME:
        {
        WMI_OPT_TX_FRAME_CMD optTxFrmCmd;
            A_UINT8 data[MAX_OPT_DATA_LEN];

            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&optTxFrmCmd, userdata,
                                      sizeof(optTxFrmCmd)))
            {
                ret = -EFAULT;
            } else if (copy_from_user(data,
                                      userdata+sizeof(WMI_OPT_TX_FRAME_CMD)-1,
                                      optTxFrmCmd.optIEDataLen))
            {
                ret = -EFAULT;
            } else {
                ret = wmi_opt_tx_frame_cmd(ar->arWmi,
                                           optTxFrmCmd.frmType,
                                           optTxFrmCmd.dstAddr,
                                           optTxFrmCmd.bssid,
                                           optTxFrmCmd.optIEDataLen,
                                           data);
            }

            break;
        }
        case AR6000_XIOCTL_WMI_SETRETRYLIMITS:
        {
            WMI_SET_RETRY_LIMITS_CMD setRetryParams;

            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&setRetryParams, userdata,
                                      sizeof(setRetryParams)))
            {
                ret = -EFAULT;
            } else {
                if (wmi_set_retry_limits_cmd(ar->arWmi, setRetryParams.frameType,
                                          setRetryParams.trafficClass,
                                          setRetryParams.maxRetries,
                                          setRetryParams.enableNotify) != A_OK)
                {
                    ret = -EIO;
                }
                AR6000_SPIN_LOCK(&ar->arLock, 0);
                ar->arMaxRetries = setRetryParams.maxRetries;
                AR6000_SPIN_UNLOCK(&ar->arLock, 0);
            }
            break;
        }

        case AR6000_XIOCTL_SET_BEACON_INTVAL:
        {
            WMI_BEACON_INT_CMD bIntvlCmd;

            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&bIntvlCmd, userdata,
                       sizeof(bIntvlCmd)))
            {
                ret = -EFAULT;
            } else if (wmi_set_adhoc_bconIntvl_cmd(ar->arWmi, bIntvlCmd.beaconInterval)
                        != A_OK)
            {
                ret = -EIO;
            }
            if(ret == 0) {
                ar->ap_beacon_interval = bIntvlCmd.beaconInterval;
                ar->ap_profile_flag = 1; /* There is a change in profile */
            }
            break;
        }
        case IEEE80211_IOCTL_SETAUTHALG:
        {
            AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
            struct ieee80211req_authalg req;

            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&req, userdata,
                       sizeof(struct ieee80211req_authalg)))
            {
                ret = -EFAULT;
            } else {
                if (req.auth_alg & AUTH_ALG_OPEN_SYSTEM) {
                    ar->arDot11AuthMode  |= OPEN_AUTH;
                    ar->arPairwiseCrypto  = NONE_CRYPT;
                    ar->arGroupCrypto     = NONE_CRYPT;
                }
                if (req.auth_alg & AUTH_ALG_SHARED_KEY) {
                    ar->arDot11AuthMode  |= SHARED_AUTH;
                    ar->arPairwiseCrypto  = WEP_CRYPT;
                    ar->arGroupCrypto     = WEP_CRYPT;
                }
                if (req.auth_alg == AUTH_ALG_LEAP) {
                    ar->arDot11AuthMode   = LEAP_AUTH;
                }
            }
            break;
        }

        case AR6000_XIOCTL_SET_VOICE_PKT_SIZE:
            ret = ar6000_xioctl_set_voice_pkt_size(dev, userdata);
            break;

        case AR6000_XIOCTL_SET_MAX_SP:
            ret = ar6000_xioctl_set_max_sp_len(dev, userdata);
            break;

        case AR6000_XIOCTL_WMI_GET_ROAM_TBL:
            ret = ar6000_ioctl_get_roam_tbl(dev, rq);
            break;
        case AR6000_XIOCTL_WMI_SET_ROAM_CTRL:
            ret = ar6000_ioctl_set_roam_ctrl(dev, userdata);
            break;
        case AR6000_XIOCTRL_WMI_SET_POWERSAVE_TIMERS:
            ret = ar6000_ioctl_set_powersave_timers(dev, userdata);
            break;
        case AR6000_XIOCTRL_WMI_GET_POWER_MODE:
            ret = ar6000_ioctl_get_power_mode(dev, rq);
            break;
        case AR6000_XIOCTRL_WMI_SET_WLAN_STATE:
        {
            AR6000_WLAN_STATE state;
            get_user(state, (unsigned int *)userdata);
            if (ar6000_set_wlan_state(ar, state)!=A_OK) {
                ret = -EIO;
            }       
            break;
        }
        case AR6000_XIOCTL_WMI_GET_ROAM_DATA:
            ret = ar6000_ioctl_get_roam_data(dev, rq);
            break;
        case AR6000_XIOCTL_WMI_SET_BT_STATUS:
            ret = ar6000_xioctl_set_bt_status_cmd(dev, userdata);
            break;
        case AR6000_XIOCTL_WMI_SET_BT_PARAMS:
            ret = ar6000_xioctl_set_bt_params_cmd(dev, userdata);
            break;
        case AR6000_XIOCTL_WMI_STARTSCAN:
        {
            WMI_START_SCAN_CMD setStartScanCmd, *cmdp;

            if (ar->arWmiReady == FALSE) {
                    ret = -EIO;
                } else if (copy_from_user(&setStartScanCmd, userdata,
                                          sizeof(setStartScanCmd)))
                {
                    ret = -EFAULT;
                } else {
                    if (setStartScanCmd.numChannels > 1) {
                        cmdp = A_MALLOC(130);
                        if (copy_from_user(cmdp, userdata,
                                           sizeof (*cmdp) +
                                           ((setStartScanCmd.numChannels - 1) *
                                           sizeof(A_UINT16))))
                        {
                            kfree(cmdp);
                            return -EFAULT;
                        }
                    } else {
                        cmdp = &setStartScanCmd;
                    }

                    if (wmi_startscan_cmd(ar->arWmi, cmdp->scanType,
                                          cmdp->forceFgScan,
                                          cmdp->isLegacy,
                                          cmdp->homeDwellTime,
                                          cmdp->forceScanInterval,
                                          cmdp->numChannels,
                                          cmdp->channelList) != A_OK)
                    {
                        ret = -EIO;
                    }
                }
            break;
        }
        case AR6000_XIOCTL_WMI_SETFIXRATES:
        {
            WMI_FIX_RATES_CMD setFixRatesCmd;
            A_STATUS returnStatus;

            if (ar->arWmiReady == FALSE) {
                    ret = -EIO;
                } else if (copy_from_user(&setFixRatesCmd, userdata,
                                          sizeof(setFixRatesCmd)))
                {
                    ret = -EFAULT;
                } else {
                    returnStatus = wmi_set_fixrates_cmd(ar->arWmi, setFixRatesCmd.fixRateMask);
                    if (returnStatus == A_EINVAL) {
                        ret = -EINVAL;
                    } else if(returnStatus != A_OK) {
                        ret = -EIO;
                    } else {
                        ar->ap_profile_flag = 1; /* There is a change in profile */
                    }
                }
            break;
        }

        case AR6000_XIOCTL_WMI_GETFIXRATES:
        {
            WMI_FIX_RATES_CMD getFixRatesCmd;
            AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
            int ret = 0;

            if (ar->bIsDestroyProgress) {
                return -EBUSY;
            }
            if (ar->arWmiReady == FALSE) {
                return -EIO;
            }

            if (down_interruptible(&ar->arSem)) {
                return -ERESTARTSYS;
            }
            if (ar->bIsDestroyProgress) {
                up(&ar->arSem);
                return -EBUSY;
            }
            /* Used copy_from_user/copy_to_user to access user space data */
            if (copy_from_user(&getFixRatesCmd, userdata, sizeof(getFixRatesCmd))) {
                ret = -EFAULT;
            } else {
                ar->arRateMask = 0xFFFF;

                if (wmi_get_ratemask_cmd(ar->arWmi) != A_OK) {
                    up(&ar->arSem);
                    return -EIO;
                }

                wait_event_interruptible_timeout(arEvent, ar->arRateMask != 0xFFFF, wmitimeout * HZ);

                if (signal_pending(current)) {
                    ret = -EINTR;
                }

                if (!ret) {
                    getFixRatesCmd.fixRateMask = ar->arRateMask;
                }

                if(copy_to_user(userdata, &getFixRatesCmd, sizeof(getFixRatesCmd))) {
                   ret = -EFAULT;
                }

                up(&ar->arSem);
            }
            break;
        }
        case AR6000_XIOCTL_WMI_SET_AUTHMODE:
        {
            WMI_SET_AUTH_MODE_CMD setAuthMode;

            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&setAuthMode, userdata,
                                      sizeof(setAuthMode)))
            {
                ret = -EFAULT;
            } else {
                if (wmi_set_authmode_cmd(ar->arWmi, setAuthMode.mode) != A_OK)
                {
                    ret = -EIO;
                }
            }
            break;
        }
        case AR6000_XIOCTL_WMI_SET_REASSOCMODE:
        {
            WMI_SET_REASSOC_MODE_CMD setReassocMode;

            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&setReassocMode, userdata,
                                      sizeof(setReassocMode)))
            {
                ret = -EFAULT;
            } else {
                if (wmi_set_reassocmode_cmd(ar->arWmi, setReassocMode.mode) != A_OK)
                {
                    ret = -EIO;
                }
            }
            break;
        }
        case AR6000_XIOCTL_DIAG_READ:
        {
            A_UINT32 addr, data;
            get_user(addr, (unsigned int *)userdata);
            addr = TARG_VTOP(ar->arTargetType, addr);
            if (ar6000_ReadRegDiag(ar->arHifDevice, &addr, &data) != A_OK) {
                ret = -EIO;
            }
            put_user(data, (unsigned int *)userdata + 1);
            break;
        }
        case AR6000_XIOCTL_DIAG_WRITE:
        {
            A_UINT32 addr, data;
            get_user(addr, (unsigned int *)userdata);
            get_user(data, (unsigned int *)userdata + 1);
            addr = TARG_VTOP(ar->arTargetType, addr);
            if (ar6000_WriteRegDiag(ar->arHifDevice, &addr, &data) != A_OK) {
                ret = -EIO;
            }
            break;
        }
        case AR6000_XIOCTL_WMI_SET_KEEPALIVE:
        {
             WMI_SET_KEEPALIVE_CMD setKeepAlive;
             if (ar->arWmiReady == FALSE) {
                 return -EIO;
             } else if (copy_from_user(&setKeepAlive, userdata,
                        sizeof(setKeepAlive))){
                 ret = -EFAULT;
             } else {
                 if (wmi_set_keepalive_cmd(ar->arWmi, setKeepAlive.keepaliveInterval) != A_OK) {
                     ret = -EIO;
               }
             }
             break;
        }
        case AR6000_XIOCTL_WMI_SET_PARAMS:
        {
             WMI_SET_PARAMS_CMD cmd;
             if (ar->arWmiReady == FALSE) {
                 return -EIO;
             } else if (copy_from_user(&cmd, userdata,
                        sizeof(cmd))){
                 ret = -EFAULT;
             } else if (copy_from_user(&cmd, userdata,
                        sizeof(cmd) + cmd.length))
            {
                ret = -EFAULT;
            } else {
                 if (wmi_set_params_cmd(ar->arWmi, cmd.opcode, cmd.length, cmd.buffer) != A_OK) {
                     ret = -EIO;
               }
             }
             break;
        }
        case AR6000_XIOCTL_WMI_SET_MCAST_FILTER:
        {
             WMI_SET_MCAST_FILTER_CMD cmd;
             if (ar->arWmiReady == FALSE) {
                 return -EIO;
             } else if (copy_from_user(&cmd, userdata,
                        sizeof(cmd))){
                 ret = -EFAULT;
             } else {
                 if (wmi_set_mcast_filter_cmd(ar->arWmi, cmd.multicast_mac[0],
                                                                                     cmd.multicast_mac[1],
                                                                                     cmd.multicast_mac[2],
                                                                                     cmd.multicast_mac[3]) != A_OK) {
                     ret = -EIO;
               }
             }
             break;
        }
        case AR6000_XIOCTL_WMI_DEL_MCAST_FILTER:
        {
             WMI_SET_MCAST_FILTER_CMD cmd;
             if (ar->arWmiReady == FALSE) {
                 return -EIO;
             } else if (copy_from_user(&cmd, userdata,
                        sizeof(cmd))){
                 ret = -EFAULT;
             } else {
                 if (wmi_del_mcast_filter_cmd(ar->arWmi, cmd.multicast_mac[0],
                                                                                     cmd.multicast_mac[1],
                                                                                     cmd.multicast_mac[2],
                                                                                     cmd.multicast_mac[3]) != A_OK) {
                     ret = -EIO;
               }
             }
             break;
        }
        case AR6000_XIOCTL_WMI_GET_KEEPALIVE:
        {
            AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
            WMI_GET_KEEPALIVE_CMD getKeepAlive;
            int ret = 0;
            if (ar->bIsDestroyProgress) {
                return -EBUSY;
            }
            if (ar->arWmiReady == FALSE) {
               return -EIO;
            }
            if (down_interruptible(&ar->arSem)) {
                return -ERESTARTSYS;
            }
            if (ar->bIsDestroyProgress) {
                up(&ar->arSem);
                return -EBUSY;
            }
            if (copy_from_user(&getKeepAlive, userdata,sizeof(getKeepAlive))) {
               ret = -EFAULT;
            } else {
            getKeepAlive.keepaliveInterval = wmi_get_keepalive_cmd(ar->arWmi);
            ar->arKeepaliveConfigured = 0xFF;
            if (wmi_get_keepalive_configured(ar->arWmi) != A_OK){
                up(&ar->arSem);
                return -EIO;
            }
            wait_event_interruptible_timeout(arEvent, ar->arKeepaliveConfigured != 0xFF, wmitimeout * HZ);
            if (signal_pending(current)) {
                ret = -EINTR;
            }

            if (!ret) {
                getKeepAlive.configured = ar->arKeepaliveConfigured;
            }
            if (copy_to_user(userdata, &getKeepAlive, sizeof(getKeepAlive))) {
               ret = -EFAULT;
            }
            up(&ar->arSem);
            }
            break;
        }
        case AR6000_XIOCTL_WMI_SET_APPIE:
        {
            WMI_SET_APPIE_CMD appIEcmd;
            A_UINT8           appIeInfo[IEEE80211_APPIE_FRAME_MAX_LEN];
            A_UINT32            fType,ieLen;

            if (ar->arWmiReady == FALSE) {
                return -EIO;
            }
            get_user(fType, (A_UINT32 *)userdata);
            appIEcmd.mgmtFrmType = fType;
            if (appIEcmd.mgmtFrmType >= IEEE80211_APPIE_NUM_OF_FRAME) {
                ret = -EIO;
            } else {
                get_user(ieLen, (A_UINT32 *)(userdata + 4));
                appIEcmd.ieLen = ieLen;
                if (appIEcmd.ieLen > IEEE80211_APPIE_FRAME_MAX_LEN) {
                    ret = -EIO;
                    break;
                }
                if (copy_from_user(appIeInfo, userdata + 8, appIEcmd.ieLen)) {
                    ret = -EFAULT;
                } else {
                    if (wmi_set_appie_cmd(ar->arWmi, appIEcmd.mgmtFrmType,
                                          appIEcmd.ieLen,  appIeInfo) != A_OK)
                    {
                        ret = -EIO;
                    }
                }
            }
            break;
        }
        case AR6000_XIOCTL_WMI_SET_MGMT_FRM_RX_FILTER:
        {
            WMI_BSS_FILTER_CMD cmd;
            A_UINT32    filterType;

            if (copy_from_user(&filterType, userdata, sizeof(A_UINT32)))
            {
                return -EFAULT;
            }
            if (filterType & (IEEE80211_FILTER_TYPE_BEACON |
                                    IEEE80211_FILTER_TYPE_PROBE_RESP))
            {
                cmd.bssFilter = ALL_BSS_FILTER;
            } else {
                cmd.bssFilter = NONE_BSS_FILTER;
            }
            if (wmi_bssfilter_cmd(ar->arWmi, cmd.bssFilter, 0) != A_OK) {
                ret = -EIO;
            }

            AR6000_SPIN_LOCK(&ar->arLock, 0);
            ar->arMgmtFilter = filterType;
            AR6000_SPIN_UNLOCK(&ar->arLock, 0);
            break;
        }
        case AR6000_XIOCTL_WMI_SET_WSC_STATUS:
        {
            A_UINT32    wsc_status;

            if (ar->arWmiReady == FALSE) {
                return -EIO;
            } else if (copy_from_user(&wsc_status, userdata, sizeof(A_UINT32)))
            {
                return -EFAULT;
            }
            if (wmi_set_wsc_status_cmd(ar->arWmi, wsc_status) != A_OK) {
                ret = -EIO;
            }
            break;
        }
        case AR6000_XIOCTL_BMI_ROMPATCH_INSTALL:
        {
            A_UINT32 ROM_addr;
            A_UINT32 RAM_addr;
            A_UINT32 nbytes;
            A_UINT32 do_activate;
            A_UINT32 rompatch_id;

            get_user(ROM_addr, (A_UINT32 *)userdata);
            get_user(RAM_addr, (A_UINT32 *)userdata + 1);
            get_user(nbytes, (A_UINT32 *)userdata + 2);
            get_user(do_activate, (A_UINT32 *)userdata + 3);
            AR_DEBUG_PRINTF("Install rompatch from ROM: 0x%x to RAM: 0x%x  length: %d\n",
                             ROM_addr, RAM_addr, nbytes);
            ret = BMIrompatchInstall(hifDevice, ROM_addr, RAM_addr,
                                        nbytes, do_activate, &rompatch_id);
            if (ret == A_OK) {
                put_user(rompatch_id, (unsigned int *)rq->ifr_data); /* return value */
            }
            break;
        }

        case AR6000_XIOCTL_BMI_ROMPATCH_UNINSTALL:
        {
            A_UINT32 rompatch_id;

            get_user(rompatch_id, (A_UINT32 *)userdata);
            AR_DEBUG_PRINTF("UNinstall rompatch_id %d\n", rompatch_id);
            ret = BMIrompatchUninstall(hifDevice, rompatch_id);
            break;
        }

        case AR6000_XIOCTL_BMI_ROMPATCH_ACTIVATE:
        case AR6000_XIOCTL_BMI_ROMPATCH_DEACTIVATE:
        {
            A_UINT32 rompatch_count;

            get_user(rompatch_count, (A_UINT32 *)userdata);
            AR_DEBUG_PRINTF("Change rompatch activation count=%d\n", rompatch_count);
            length = sizeof(A_UINT32) * rompatch_count;
            if ((buffer = (unsigned char *)A_MALLOC(length)) != NULL) {
                A_MEMZERO(buffer, length);
                if (copy_from_user(buffer, &userdata[sizeof(rompatch_count)], length))
                {
                    ret = -EFAULT;
                } else {
                    if (cmd == AR6000_XIOCTL_BMI_ROMPATCH_ACTIVATE) {
                        ret = BMIrompatchActivate(hifDevice, rompatch_count, (A_UINT32 *)buffer);
                    } else {
                        ret = BMIrompatchDeactivate(hifDevice, rompatch_count, (A_UINT32 *)buffer);
                    }
                }
                A_FREE(buffer);
            } else {
                ret = -ENOMEM;
            }

            break;
        }
        case AR6000_XIOCTL_SET_IP:
        {
            WMI_SET_IP_CMD setIP;

            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&setIP, userdata,
                                      sizeof(setIP)))
            {
                ret = -EFAULT;
            } else {
                if (wmi_set_ip_cmd(ar->arWmi,
                                &setIP) != A_OK)
                {
                    ret = -EIO;
                }
            }
            break;
        }

        case AR6000_XIOCTL_WMI_SET_HOST_SLEEP_MODE:
        {
            WMI_SET_HOST_SLEEP_MODE_CMD setHostSleepMode;

            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&setHostSleepMode, userdata,
                                      sizeof(setHostSleepMode)))
            {
                ret = -EFAULT;
            } else {
                if (wmi_set_host_sleep_mode_cmd(ar->arWmi,
                                &setHostSleepMode) != A_OK)
                {
                    ret = -EIO;
                }
            }
            break;
        }
        case AR6000_XIOCTL_WMI_SET_WOW_MODE:
        {
            WMI_SET_WOW_MODE_CMD setWowMode;

            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&setWowMode, userdata,
                                      sizeof(setWowMode)))
            {
                ret = -EFAULT;
            } else {
                if (wmi_set_wow_mode_cmd(ar->arWmi,
                                &setWowMode) != A_OK)
                {
                    ret = -EIO;
                }
            }
            break;
        }
        case AR6000_XIOCTL_WMI_GET_WOW_LIST:
        {
            WMI_GET_WOW_LIST_CMD getWowList;

            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&getWowList, userdata,
                                      sizeof(getWowList)))
            {
                ret = -EFAULT;
            } else {
                if (wmi_get_wow_list_cmd(ar->arWmi,
                                &getWowList) != A_OK)
                {
                    ret = -EIO;
                }
            }
            break;
        }
        case AR6000_XIOCTL_WMI_ADD_WOW_PATTERN:
        {
#define WOW_PATTERN_SIZE 64
#define WOW_MASK_SIZE 64

            WMI_ADD_WOW_PATTERN_CMD cmd;
            A_UINT8 mask_data[WOW_PATTERN_SIZE]={0};
            A_UINT8 pattern_data[WOW_PATTERN_SIZE]={0};

            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else {

                if(copy_from_user(&cmd, userdata,
                            sizeof(WMI_ADD_WOW_PATTERN_CMD)))
                      return -EFAULT;
                if (copy_from_user(pattern_data,
                                      userdata + 3,
                                      cmd.filter_size)){
                        ret = -EFAULT;
                        break;
                }
                if (copy_from_user(mask_data,
                                      (userdata + 3 + cmd.filter_size),
                                      cmd.filter_size)){
                        ret = -EFAULT;
                        break;
                } else {
                    if (wmi_add_wow_pattern_cmd(ar->arWmi,
                                &cmd, pattern_data, mask_data, cmd.filter_size) != A_OK){
                        ret = -EIO;
                    }
                }
            }
#undef WOW_PATTERN_SIZE
#undef WOW_MASK_SIZE
            break;
        }
        case AR6000_XIOCTL_WMI_DEL_WOW_PATTERN:
        {
            WMI_DEL_WOW_PATTERN_CMD delWowPattern;

            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&delWowPattern, userdata,
                                      sizeof(delWowPattern)))
            {
                ret = -EFAULT;
            } else {
                if (wmi_del_wow_pattern_cmd(ar->arWmi,
                                &delWowPattern) != A_OK)
                {
                    ret = -EIO;
                }
            }
            break;
        }
        case AR6000_XIOCTL_DUMP_HTC_CREDIT_STATE:
            if (ar->arHtcTarget != NULL) {
                HTCDumpCreditStates(ar->arHtcTarget);
            }
            break;
        case AR6000_XIOCTL_TRAFFIC_ACTIVITY_CHANGE:
            if (ar->arHtcTarget != NULL) {
                struct ar6000_traffic_activity_change data;

                if (copy_from_user(&data, userdata, sizeof(data)))
                {
                    return -EFAULT;
                }
                    /* note, this is used for testing (mbox ping testing), indicate activity
                     * change using the stream ID as the traffic class */
                ar6000_indicate_tx_activity(ar,
                                            (A_UINT8)data.StreamID,
                                            data.Active ? TRUE : FALSE);
            }
            break;
        case AR6000_XIOCTL_WMI_SET_CONNECT_CTRL_FLAGS:
            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&connectCtrlFlags, userdata,
                                      sizeof(connectCtrlFlags)))
            {
                ret = -EFAULT;
            } else {
                ar->arConnectCtrlFlags = connectCtrlFlags;
            }
            break;
        case AR6000_XIOCTL_WMI_SET_AKMP_PARAMS:
            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&akmpParams, userdata,
                                      sizeof(WMI_SET_AKMP_PARAMS_CMD)))
            {
                ret = -EFAULT;
            } else {
                if (wmi_set_akmp_params_cmd(ar->arWmi, &akmpParams) != A_OK) {
                    ret = -EIO;
                }
            }
            break;
        case AR6000_XIOCTL_WMI_SET_PMKID_LIST:
            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else {
                if (copy_from_user(&pmkidInfo.numPMKID, userdata,
                                      sizeof(pmkidInfo.numPMKID)))
                {
                    ret = -EFAULT;
                    break;
                }
                if (copy_from_user(&pmkidInfo.pmkidList,
                                   userdata + sizeof(pmkidInfo.numPMKID),
                                   pmkidInfo.numPMKID * sizeof(WMI_PMKID)))
                {
                    ret = -EFAULT;
                    break;
                }
                if (wmi_set_pmkid_list_cmd(ar->arWmi, &pmkidInfo) != A_OK) {
                    ret = -EIO;
                }
            }
            break;
        case AR6000_XIOCTL_WMI_GET_PMKID_LIST:
            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else  {
                if (wmi_get_pmkid_list_cmd(ar->arWmi) != A_OK) {
                    ret = -EIO;
                }
            }
            break;
        case AR6000_XIOCTL_WMI_ABORT_SCAN:
            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            }
            ret = wmi_abort_scan_cmd(ar->arWmi);
            break;
        case AR6000_XIOCTL_AP_HIDDEN_SSID:
        {
            A_UINT8    hidden_ssid;
            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&hidden_ssid, userdata, sizeof(hidden_ssid))) {
                ret = -EFAULT;
            } else {
                wmi_ap_set_hidden_ssid(ar->arWmi, hidden_ssid);
                ar->ap_hidden_ssid = hidden_ssid;
                ar->ap_profile_flag = 1; /* There is a change in profile */
            }
            break;
        }
        case AR6000_XIOCTL_AP_GET_STA_LIST:
        {
            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else {
                A_UINT8 i;
                ap_get_sta_t temp;
                A_MEMZERO(&temp, sizeof(temp));
                for(i=0;i<AP_MAX_NUM_STA;i++) {
                    A_MEMCPY(temp.sta[i].mac, ar->sta_list[i].mac, ATH_MAC_LEN);
                    temp.sta[i].aid = ar->sta_list[i].aid;
                }
                if(copy_to_user((ap_get_sta_t *)rq->ifr_data, &temp,
                                 sizeof(ar->sta_list))) {
                    ret = -EFAULT;
                }
            }
            break;
        }
        case AR6000_XIOCTL_AP_SET_NUM_STA:
        {
            A_UINT8    num_sta;
            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&num_sta, userdata, sizeof(num_sta))) {
                ret = -EFAULT;
            } else if(num_sta > AP_MAX_NUM_STA) {
                /* value out of range */
                ret = -EINVAL;
            } else {
                wmi_ap_set_num_sta(ar->arWmi, num_sta);
            }
            break;
        }
        case AR6000_XIOCTL_AP_SET_ACL_POLICY:
        {
            A_UINT8    policy;
            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&policy, userdata, sizeof(policy))) {
                ret = -EFAULT;
            } else if(policy == ar->g_acl.policy) {
                /* No change in policy */
            } else {
                if(!(policy & AP_ACL_RETAIN_LIST_MASK)) {
                    /* clear ACL list */
                    memset(&ar->g_acl,0,sizeof(WMI_AP_ACL));
                }
                ar->g_acl.policy = policy;
                wmi_ap_set_acl_policy(ar->arWmi, policy);
            }
            break;
        }
        case AR6000_XIOCTL_AP_SET_ACL_MAC:
        {
            WMI_AP_ACL_MAC_CMD    acl;
            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&acl, userdata, sizeof(acl))) {
                ret = -EFAULT;
            } else {
                if(acl_add_del_mac(&ar->g_acl, &acl)) {
                    wmi_ap_acl_mac_list(ar->arWmi, &acl);
                } else {
                    A_PRINTF("ACL list error\n");
                    ret = -EIO;
                }
            }
            break;
        }
        case AR6000_XIOCTL_AP_GET_ACL_LIST:
        {
            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if(copy_to_user((WMI_AP_ACL *)rq->ifr_data, &ar->g_acl,
                                 sizeof(WMI_AP_ACL))) {
                    ret = -EFAULT;
            }
            break;
        }
        case AR6000_XIOCTL_AP_COMMIT_CONFIG:
        {
            ret = ar6000_ap_mode_profile_commit(ar);
            break;
        }
        case IEEE80211_IOCTL_GETWPAIE:
        {
            struct ieee80211req_wpaie wpaie;
            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&wpaie, userdata, sizeof(wpaie))) {
                ret = -EFAULT;
            } else if (ar6000_ap_mode_get_wpa_ie(ar, &wpaie)) {
                ret = -EFAULT;
            } else if(copy_to_user(userdata, &wpaie, sizeof(wpaie))) {
                ret = -EFAULT;
            }
            break;
        }
        case AR6000_XIOCTL_AP_CONN_INACT_TIME:
        {
            A_UINT32    period;
            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&period, userdata, sizeof(period))) {
                ret = -EFAULT;
            } else {
                wmi_ap_conn_inact_time(ar->arWmi, period);
            }
            break;
        }
        case AR6000_XIOCTL_AP_PROT_SCAN_TIME:
        {
            WMI_AP_PROT_SCAN_TIME_CMD  bgscan;
            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&bgscan, userdata, sizeof(bgscan))) {
                ret = -EFAULT;
            } else {
                wmi_ap_bgscan_time(ar->arWmi, bgscan.period_min, bgscan.dwell_ms);
            }
            break;
        }
        case AR6000_XIOCTL_SET_COUNTRY:
        {
            ret = ar6000_ioctl_set_country(dev, rq);
            break;
        }
        case AR6000_XIOCTL_AP_SET_DTIM:
        {
            WMI_AP_SET_DTIM_CMD  d;
            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&d, userdata, sizeof(d))) {
                ret = -EFAULT;
            } else {
                if(d.dtim > 0 && d.dtim < 11) {
                    ar->ap_dtim_period = d.dtim;
                    wmi_ap_set_dtim(ar->arWmi, d.dtim);
                    ar->ap_profile_flag = 1; /* There is a change in profile */
                } else {
                    A_PRINTF("DTIM out of range. Valid range is [1-10]\n");
                    ret = -EIO;
                }
            }
            break;
        }
        case AR6000_XIOCTL_WMI_TARGET_EVENT_REPORT:
        {
            WMI_SET_TARGET_EVENT_REPORT_CMD evtCfgCmd;

            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            }
            if (copy_from_user(&evtCfgCmd, userdata,
                               sizeof(evtCfgCmd))) {
                ret = -EFAULT;
                break;
            }
            ret = wmi_set_target_event_report_cmd(ar->arWmi, &evtCfgCmd);
            break;
        }
        case AR6000_XIOCTL_AP_INTRA_BSS_COMM:
        {
            A_UINT8    intra=0;
            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if (copy_from_user(&intra, userdata, sizeof(intra))) {
                ret = -EFAULT;
            } else {
                ar->intra_bss = (intra?1:0);
            }
            break;
        }
        case AR6000_XIOCTL_AP_GET_HIDDEN_SSID:
        {
            WMI_AP_HIDDEN_SSID_CMD ssid;
            ssid.hidden_ssid = ar->ap_hidden_ssid;

            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if(copy_to_user((WMI_AP_HIDDEN_SSID_CMD *)rq->ifr_data,
                                    &ssid, sizeof(WMI_AP_HIDDEN_SSID_CMD))) {
                    ret = -EFAULT;
            }
            break;
        }
        case AR6000_XIOCTL_AP_GET_COUNTRY:
        {
            WMI_SET_COUNTRY_CMD cty;
            A_MEMCPY(cty.countryCode, ar->country_code, 3);

            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if(copy_to_user((WMI_SET_COUNTRY_CMD *)rq->ifr_data,
                                    &cty, sizeof(WMI_SET_COUNTRY_CMD))) {
                    ret = -EFAULT;
            }
            break;
        }
        case AR6000_XIOCTL_AP_GET_WMODE:
        {
            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if(copy_to_user((A_UINT8 *)rq->ifr_data,
                                    &ar->ap_wmode, sizeof(A_UINT8))) {
                    ret = -EFAULT;
            }
            break;
        }
        case AR6000_XIOCTL_AP_GET_DTIM:
        {
            WMI_AP_SET_DTIM_CMD dtim;
            dtim.dtim = ar->ap_dtim_period;

            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if(copy_to_user((WMI_AP_SET_DTIM_CMD *)rq->ifr_data,
                                    &dtim, sizeof(WMI_AP_SET_DTIM_CMD))) {
                    ret = -EFAULT;
            }
            break;
        }
        case AR6000_XIOCTL_AP_GET_BINTVL:
        {
            WMI_BEACON_INT_CMD bi;
            bi.beaconInterval = ar->ap_beacon_interval;

            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if(copy_to_user((WMI_BEACON_INT_CMD *)rq->ifr_data,
                                    &bi, sizeof(WMI_BEACON_INT_CMD))) {
                    ret = -EFAULT;
            }
            break;
        }
        case AR6000_XIOCTL_AP_GET_RTS:
        {
            WMI_SET_RTS_CMD rts;
            rts.threshold = ar->arRTS;

            if (ar->arWmiReady == FALSE) {
                ret = -EIO;
            } else if(copy_to_user((WMI_SET_RTS_CMD *)rq->ifr_data,
                                    &rts, sizeof(WMI_SET_RTS_CMD))) {
                    ret = -EFAULT;
            }
            break;
        }
        default:
            ret = -EOPNOTSUPP;
    }
    return ret;
}

A_UINT8 mac_cmp_wild(A_UINT8 *mac, A_UINT8 *new_mac, A_UINT8 wild, A_UINT8 new_wild)
{
    A_UINT8 i;

    for(i=0;i<ATH_MAC_LEN;i++) {
        if((wild & 1<<i) && (new_wild & 1<<i)) continue;
        if(mac[i] != new_mac[i]) return 1;
    }
    if((A_MEMCMP(new_mac, null_mac, 6)==0) && new_wild &&
        (wild != new_wild)) {
        return 1;
    }

    return 0;
}

A_UINT8    acl_add_del_mac(WMI_AP_ACL *a, WMI_AP_ACL_MAC_CMD *acl)
{
    A_INT8    already_avail=-1, free_slot=-1, i;

    /* To check whether this mac is already there in our list */
    for(i=AP_ACL_SIZE-1;i>=0;i--)
    {
        if(mac_cmp_wild(a->acl_mac[i], acl->mac, a->wildcard[i],
            acl->wildcard)==0)
                already_avail = i;

        if(!((1 << i) & a->index))
            free_slot = i;
    }

    if(acl->action == ADD_MAC_ADDR)
    {
        /* Dont add mac if it is already available */
        if((already_avail >= 0) || (free_slot == -1))
            return 0;

        A_MEMCPY(a->acl_mac[free_slot], acl->mac, ATH_MAC_LEN);
        a->index = a->index | (1 << free_slot);
        acl->index = free_slot;
        a->wildcard[free_slot] = acl->wildcard;
        return 1;
    }
    else if(acl->action == DEL_MAC_ADDR)
    {
        if(acl->index > AP_ACL_SIZE)
            return 0;

        if(!(a->index & (1 << acl->index)))
            return 0;

        A_MEMZERO(a->acl_mac[acl->index],ATH_MAC_LEN);
        a->index = a->index & ~(1 << acl->index);
        a->wildcard[acl->index] = 0;
        return 1;
    }

    return 0;
}
