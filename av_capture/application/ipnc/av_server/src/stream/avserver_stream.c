#include <avserver.h>
#include <stream.h>
#include <encode.h>

GlobalData gbl = { 0, 0, 0, 0, 0, NOSTD };
static int IsDrawDateTime  = 0;

int VIDEO_streamSysInit()
{
  int status;
  STREAM_SET streamSet;

  pthread_mutex_init(&gbl.mutex, NULL);

  memset(&streamSet, 0, sizeof(streamSet));

  streamSet.ImageWidth  = gAVSERVER_config.encodeConfig[0].cropWidth;
  streamSet.ImageHeight = gAVSERVER_config.encodeConfig[0].cropHeight;
  streamSet.Mpeg4Quality = gAVSERVER_config.encodeConfig[0].codecBitrate;
  streamSet.JpgQuality   = gAVSERVER_config.encodeConfig[1].qValue;

  switch(gAVSERVER_config.encodeConfig[0].codecType) {
    case ALG_VID_CODEC_H264:
    case ALG_VID_CODEC_MPEG4:
      break;
    case ALG_VID_CODEC_MJPEG:
      streamSet.ImageWidth_Ext[STREAM_EXT_JPG] = gAVSERVER_config.encodeConfig[0].cropWidth;
      streamSet.ImageHeight_Ext[STREAM_EXT_JPG] = gAVSERVER_config.encodeConfig[0].cropHeight;
      break;

  }

  if(gAVSERVER_config.numEncodeStream>1) {
    switch(gAVSERVER_config.encodeConfig[1].codecType) {
      case ALG_VID_CODEC_H264:
      case ALG_VID_CODEC_MPEG4:
        streamSet.ImageWidth_Ext[STREAM_EXT_MP4_CIF] = gAVSERVER_config.encodeConfig[1].cropWidth;
        streamSet.ImageHeight_Ext[STREAM_EXT_MP4_CIF] = gAVSERVER_config.encodeConfig[1].cropHeight;
        break;
      case ALG_VID_CODEC_MJPEG:
        streamSet.ImageWidth_Ext[STREAM_EXT_JPG] = gAVSERVER_config.encodeConfig[1].cropWidth;
        streamSet.ImageHeight_Ext[STREAM_EXT_JPG] = gAVSERVER_config.encodeConfig[1].cropHeight;
        break;

    }
  }

  if(gAVSERVER_config.numEncodeStream>2) {
    switch(gAVSERVER_config.encodeConfig[2].codecType) {
      case ALG_VID_CODEC_H264:
      case ALG_VID_CODEC_MPEG4:
        streamSet.ImageWidth_Ext[STREAM_EXT_MP4_CIF] = gAVSERVER_config.encodeConfig[2].cropWidth;
        streamSet.ImageHeight_Ext[STREAM_EXT_MP4_CIF] = gAVSERVER_config.encodeConfig[2].cropHeight;
        break;
      case ALG_VID_CODEC_MJPEG:
        streamSet.ImageWidth_Ext[STREAM_EXT_JPG] = gAVSERVER_config.encodeConfig[2].cropWidth;
        streamSet.ImageHeight_Ext[STREAM_EXT_JPG] = gAVSERVER_config.encodeConfig[2].cropHeight;
        break;

    }
  }

#ifdef AVSERVER_DEBUG_VIDEO_STREAM_THR
  {
	  int cnt;
	  for(cnt = 0; cnt < STREAM_EXT_NUM; cnt++)
	  {
		OSA_printf(" STREAM: Ext %d: %dx%d\n", cnt, streamSet.ImageWidth_Ext[cnt], streamSet.ImageHeight_Ext[cnt]);
	  }
  }
#endif

  streamSet.Mem_layout	= gAVSERVER_config.streamConfig.mem_layou_set;

  status = stream_init( stream_get_handle(), &streamSet);

  if(status!=OSA_SOK)
    OSA_ERROR("stream_init()\n");

  return status;
}

int VIDEO_streamSysExit()
{
  stream_end( stream_get_handle() );

  pthread_mutex_destroy(&gbl.mutex);

  return OSA_SOK;
}

int VIDEO_streamShmCopy(int streamId, OSA_BufInfo *pBufInfo)
{
  int  status=OSA_SOK;
  VIDEO_BufHeader *pInBufHeader;
  int frameType, streamType = -1;
  Uint32 timestamp;
  Uint32 temporalId;

  pInBufHeader = (VIDEO_BufHeader *)pBufInfo->virtAddr;

  switch(gAVSERVER_config.encodeConfig[streamId].codecType)
  {
    case ALG_VID_CODEC_H264:
      if(streamId==0)
        streamType = STREAM_H264_1;
      else
        streamType = STREAM_H264_2;
      break;
    case ALG_VID_CODEC_MPEG4:
      if(streamId==0)
        streamType = STREAM_MP4;
      else
        streamType = STREAM_MP4_EXT;
      break;
    case ALG_VID_CODEC_MJPEG:
      streamType = STREAM_MJPG;
      break;
  }

  switch(pInBufHeader->encFrameType) {
		case VIDEO_ENC_FRAME_TYPE_KEY_FRAME:
			frameType = 1;
			break;
		case VIDEO_ENC_FRAME_TYPE_NON_KEY_FRAME:
			frameType = 2;
			break;
		default:
			frameType = -1;
			break;
  }

  timestamp = pInBufHeader->timestamp;
  temporalId = pInBufHeader->temporalId;

  status = stream_write(
        pBufInfo->virtAddr+VIDEO_BUF_HEADER_SIZE,
        pInBufHeader->encFrameSize,
        frameType,
        streamType,
        timestamp,
        temporalId,
        stream_get_handle()
      );

  if(status!=OSA_SOK) {
    OSA_ERROR("stream_write(%d, %d, %d, %d %d)\n",
        pInBufHeader->encFrameSize,
        frameType,
        streamType,
        timestamp,
        temporalId
      );
  }

  return status;
}

int AUDIO_streamShmCopy(AUDIO_BufInfo *pBufInfo)
{
  int  status=OSA_SOK;


  status = stream_write(
        pBufInfo->virtAddr,
        pBufInfo->encFrameSize,
        AUDIO_FRAME,
        STREAM_AUDIO,
        pBufInfo->timestamp,
        NULL,
        stream_get_handle()
      );

  if(status!=OSA_SOK) {
    OSA_ERROR("stream_write(%d, %d, %d, %u)\n",
        pBufInfo->encFrameSize,
        AUDIO_FRAME,
        STREAM_AUDIO,
        pBufInfo->timestamp
      );
  }

  return status;
}

void SetPtzCmd(int Getcmd)
{

}

void SetDrawDateTimeStatus( int IsDraw )
{
	IsDrawDateTime = IsDraw;

	if(IsDrawDateTime > 0)
		AVSERVER_swosdEnableDateTimeStamp(IsDrawDateTime);
	else
		AVSERVER_swosdEnableDateTimeStamp(FALSE);

}

void SetOsdText(char *strText, int nLength)
{
	AVSERVER_swosdText(strText, nLength);
}

void SetOsdTextEnable(int enable)
{
	AVSERVER_swosdEnableText(enable);
}

void SetOsdLogoEnable(int enable)
{
	AVSERVER_swosdEnableLogo(enable);
}

void SetHistogramEnable(int enable)
{
	AVSERVER_histEnable(enable);
}

void SetROICfgEnable(int value)
{
	if(value==ROI_FD_CFG){
		AVSERVER_fdROIEnable(1);
	}
	else if(value==ROI_CENTER_CFG){
		AVSERVER_fxdROIEnable(1);
	}
	else {
		AVSERVER_fdROIEnable(0);
		AVSERVER_fxdROIEnable(0);
	}
}

int SetDispInterface(int type)
{
	AVSERVER_SetDisplay(type);
	return 0;
}

void VIDEO_frameRate_setparm( int streamId, unsigned int frameRate )
{
	AVSERVER_setEncFramerate(streamId, frameRate);
}

void VIDEO_jpeg_quality_setparm( int quality )
{
  int streamId;

  for(streamId=0; streamId<gAVSERVER_config.numEncodeStream; streamId++) {
    if(gAVSERVER_config.encodeConfig[streamId].codecType==ALG_VID_CODEC_MJPEG) {
      AVSERVER_setEncBitrate(streamId, quality);
    }
  }
}

void VIDEO_bitRate_setparm( int type, int bitrate )
{
  int streamId=type;

  if(gAVSERVER_config.encodeConfig[streamId].codecType!=ALG_VID_CODEC_MJPEG) {
     AVSERVER_setEncBitrate(streamId, bitrate);
  }
  else {
     AVSERVER_setEncBitrate(streamId+1, bitrate);
  }
}

void AUDIO_aac_bitRate_setparm(int bitrate)
{
	AVSERVER_setAACBitrate(bitrate);
}

void VIDEO_motion_setparm(ApproMotionPrm* pMotionPrm)
{
	AVSERVER_setMotion(pMotionPrm->bMotionEnable, pMotionPrm->bMotionCEnale,
		 pMotionPrm->MotionCValue, pMotionPrm->MotionLevel, pMotionPrm->MotionBlock);
}

int VIDEO_streamGetJPGSerialNum(void)
{
	return stream_get_handle()->MemInfo.video_info[VIDOE_INFO_MJPG].cur_serial;

}

int VIDEO_streamGetMediaInfo(void)
{
	return((gAVSERVER_config.sensorFps>30)?1:0);
}

void VIDEO_streamSetVNFStatus(unsigned int streamId, unsigned int value)
{
	if(value&FFLAG_SNF)
		AVSERVER_snfEnable(streamId, 1);
	else
		AVSERVER_snfEnable(streamId, 0);

	if(value&FFLAG_TNF)
		AVSERVER_tnfEnable(streamId, 1);
	else
		AVSERVER_tnfEnable(streamId, 0);
}

void VIDEO_streamSetFace( FaceDetectParam *faceParam )
{
	DRV_FaceDetectRunType faceType;

	faceType.type = FACE_T_NO_DETECT;

	/* Disable Face Detect for 5MP */
	if((gAVSERVER_config.sensorMode==DRV_IMGS_SENSOR_MODE_2592x1920)
		||(gAVSERVER_config.captureConfig[gAVSERVER_config.numCaptureStream-1].width>320)
		||(gAVSERVER_config.captureConfig[gAVSERVER_config.numCaptureStream-1].height!=192)
		||(gAVSERVER_config.captureConfig[gAVSERVER_config.numCaptureStream-1].width==0)
		||(gAVSERVER_config.sensorFps>AVSERVER_MAX_FRAMERATE))
	{
		AVSERVER_faceDetectEnable(DISABLE);
 		AVSERVER_faceTrackerEnable(DISABLE);
		AVSERVER_pMaskEnable(DISABLE);
		AVSERVER_faceRecogClear();
	}
	else {
		if(faceParam->pmask)
		{
			faceType.type = FACE_T_MASK;
			AVSERVER_pMaskEnable(ENABLE);
			if(faceParam->fdetect>1)
 				AVSERVER_faceTrackerEnable(ENABLE);
 			else
 				AVSERVER_faceTrackerEnable(DISABLE);
		}
		else if(faceParam->fdetect)
		{
			faceType.type = FACE_T_DETECT;
			AVSERVER_faceDetectEnable(ENABLE);
			if(faceParam->fdetect>1)
 				AVSERVER_faceTrackerEnable(ENABLE);
 			else
 				AVSERVER_faceTrackerEnable(DISABLE);
		}
		else {
			AVSERVER_faceDetectEnable(DISABLE);
			AVSERVER_pMaskEnable(DISABLE);
 			AVSERVER_faceTrackerEnable(DISABLE);
		}

		if(faceParam->frecog == FACE_RECOGIZE ){
			faceType.type = FACE_T_RECOGIZE;
			AVSERVER_faceRecogIdentifyEnable(ENABLE);
		}
		else if(faceParam->frecog == FACE_REGUSER) {
			faceType.type = FACE_T_REGUSER;
			AVSERVER_faceRegUsrEnable(ENABLE);
		}
		else if(faceParam->frecog == FACE_DELUSER){
			faceType.type = FACE_T_DELUSER;
			AVSERVER_faceRegUsrDelete(ENABLE);
		}
		else {
			AVSERVER_faceRecogClear();
		}
	}

	gAVSERVER_config.faceDetectConfig.startX = faceParam->startX;
    gAVSERVER_config.faceDetectConfig.startY = faceParam->startY;
    gAVSERVER_config.faceDetectConfig.width = faceParam->width;
    gAVSERVER_config.faceDetectConfig.height = faceParam->height;
    gAVSERVER_config.faceDetectConfig.fdconflevel = faceParam->fdconflevel;
    gAVSERVER_config.faceDetectConfig.fddirection =	faceParam->fddirection;
    gAVSERVER_config.faceDetectConfig.frconflevel = faceParam->frconflevel;
    gAVSERVER_config.faceDetectConfig.frdatabase = faceParam->frdatabase;
    gAVSERVER_config.faceDetectConfig.maskoption = faceParam->maskoption;

	VIDEO_fdSetFaceType(&faceType);
}

void VIDEO_streamSetDateTimePrm(DateTimePrm* datetimeParam)
{
	int count;

	for(count = 0; count<AVSERVER_MAX_STREAMS;count++) {
		gAVSERVER_config.captureConfig[count].swosdConfig.swosdDateFormat 		= datetimeParam->dateFormat;
		gAVSERVER_config.captureConfig[count].swosdConfig.swosdDatePos 			= datetimeParam->datePos;
		gAVSERVER_config.captureConfig[count].swosdConfig.swosdTimeFormat 		= datetimeParam->timeFormat;
		gAVSERVER_config.captureConfig[count].swosdConfig.swosdTimePos 			= datetimeParam->timePos;
		gAVSERVER_config.captureConfig[count].swosdConfig.swosdDateTimeUpdate 	= OSD_YES;
	}
}

void VIDEO_streamOsdPrm(OSDPrm* osdPrm, int id)
{
	//int enable = osdPrm->dateEnable | osdPrm->timeEnable | osdPrm->logoEnable | osdPrm->textEnable | osdPrm->detailedInfo;

	gAVSERVER_config.captureConfig[id].swosdConfig.swosdDateEnable 		= osdPrm->dateEnable;
	gAVSERVER_config.captureConfig[id].swosdConfig.swosdTimeEnable 		= osdPrm->timeEnable;

	gAVSERVER_config.captureConfig[id].swosdConfig.swosdLogoEnable 		= osdPrm->logoEnable;
	gAVSERVER_config.captureConfig[id].swosdConfig.swosdLogoPos 		= osdPrm->logoPos;

	gAVSERVER_config.captureConfig[id].swosdConfig.swosdTextEnable 		= osdPrm->textEnable;
	gAVSERVER_config.captureConfig[id].swosdConfig.swosdTextPos 		= osdPrm->textPos;

	memcpy(gAVSERVER_config.captureConfig[id].swosdConfig.swosdDispString,osdPrm->text,strlen(osdPrm->text));
	gAVSERVER_config.captureConfig[id].swosdConfig.swosdDispString[strlen(osdPrm->text)] = '\0';

	if(osdPrm->logoEnable || osdPrm->textEnable)
		gAVSERVER_config.captureConfig[id].swosdConfig.swosdLogoStringUpdate = OSD_YES;

	gAVSERVER_config.captureConfig[id].swosdConfig.swosdType	= (osdPrm->detailedInfo==1)?SWOSD_DETAIL:SWOSD_BASIC;

	//AVSERVER_swosdEnable(id, enable);
	//AVSERVER_afEnable(osdPrm->detailedInfo);
}

void VIDEO_streamSetOSDEnable(int value)
{
	AVSERVER_swosdEnable(0, (value&1)>>0);
	AVSERVER_swosdEnable(1, (value&2)>>1);
	AVSERVER_swosdEnable(2, (value&4)>>2);
}

void VIDEO_streamROIPrm(CodecROIPrm* codecROIPrm, int id)
{
	int i =0;

	gAVSERVER_config.encodeConfig[id].numROI 	= codecROIPrm->numROI;

	for(i=0;i<AVSERVER_MAX_STREAMS;i++) {
		gAVSERVER_config.encodeConfig[id].prmROI[i].startx 	= codecROIPrm->roi[i].startx;
		gAVSERVER_config.encodeConfig[id].prmROI[i].starty 	= codecROIPrm->roi[i].starty;
		gAVSERVER_config.encodeConfig[id].prmROI[i].width 	= codecROIPrm->roi[i].width;
		gAVSERVER_config.encodeConfig[id].prmROI[i].height 	= codecROIPrm->roi[i].height;
	}
}

void VIDEO_codecAdvPrm(CodecAdvPrm* codecAdvPrm, int id)
{
	gAVSERVER_config.encodeConfig[id].newCodecPrm	= (Uint16)TRUE;
	gAVSERVER_config.encodeConfig[id].ipRatio 		= (Uint16)codecAdvPrm->ipRatio;
	gAVSERVER_config.encodeConfig[id].fIframe 		= (Uint16)codecAdvPrm->fIframe;
	gAVSERVER_config.encodeConfig[id].qpInit 		= (Uint16)codecAdvPrm->qpInit;
	gAVSERVER_config.encodeConfig[id].qpMin 		= (Uint16)codecAdvPrm->qpMin;
	gAVSERVER_config.encodeConfig[id].qpMax 		= (Uint16)codecAdvPrm->qpMax;
	gAVSERVER_config.encodeConfig[id].encodePreset	= (Uint16)codecAdvPrm->meConfig;
	gAVSERVER_config.encodeConfig[id].packetSize 	= (Uint16)codecAdvPrm->packetSize;
}

void VIDEO_codecReset(int enable)
{
	int count;

	if(enable) {
		for(count = 0; count<AVSERVER_MAX_STREAMS;count++) {
			gAVSERVER_config.encodeConfig[count].resetCodec		= (Uint16)enable;
			gAVSERVER_config.encodeConfig[count].fIframe 		= (Uint16)enable;
		}
	}
}

void VIDEO_aewbSetType(int value)
{
	AVSERVER_aewbSetType(value);
}

void VIDEO_aewbPriority(int value)
{
	AVSERVER_aewbPriority(value);
}

void VIDEO_dynRangePrm(DynRangePrm* dynRangePrm)
{
	AVSERVER_saldreEnable(dynRangePrm->enable);

	if(dynRangePrm->enable) {
		AVSERVER_saldreMode(dynRangePrm->mode);
		AVSERVER_saldreLevel(dynRangePrm->level);
	}
	else {
		AVSERVER_saldreMode(SALDRE_OFF);
		AVSERVER_saldreLevel(SALDRE_LOW);
	}
}

void VIDEO_mirrorPrm(int value)
{
	AVSERVER_setMirrValue(value);
}

int SetSnapName(char *strText, int nLength)
{
	return AVSERVER_snapName(strText, nLength);
}

void SetSnapLocation(int value)
{
	AVSERVER_snapLocation(value);
}

void AUDIO_alarmLevel(int value)
{
	AVSERVER_audioAlarmLevel(value);
}

void AUDIO_alarmFlag(int enable)
{
	AVSERVER_audioAlarmEnable(enable);
}
