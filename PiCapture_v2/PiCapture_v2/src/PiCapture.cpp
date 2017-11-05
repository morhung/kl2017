// Add thu vien camera Raspberry Pi
#include <stdio.h>
#include <stdlib.h>
#include <opencv2/opencv.hpp>
#include "bcm_host.h"
#include "interface/mmal/mmal.h"
#include "interface/mmal/mmal_logging.h"
#include "interface/mmal/util/mmal_default_components.h"
#include "interface/mmal/util/mmal_connection.h"
#include "interface/mmal/util/mmal_util.h"
#include "interface/mmal/util/mmal_util_params.h"
#include "PiCapture.h"

#define VIDEO_FRAME_RATE_NUM 30
#define VIDEO_FRAME_RATE_DEN 1
#define VIDEO_OUTPUT_BUFFERS_NUM 3
#define MMAL_CAMERA_PREVIEW_PORT 0
#define MMAL_CAMERA_VIDEO_PORT 1
#define MMAL_CAMERA_CAPTURE_PORT 2

using namespace cv;
using namespace std;

int PiCapture::width = 0;
int PiCapture::height = 0;
MMAL_POOL_T * PiCapture::camera_video_port_pool = NULL;
Mat PiCapture::image = Mat();
IplImage *py, *pu, *pv, *pu_big, *pv_big, *img,* dstImage;

static void color_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer) {
	MMAL_BUFFER_HEADER_T *new_buffer;
	if(buffer->data)
	{
	    mmal_buffer_header_mem_lock(buffer);
	    int h = PiCapture::height;
	    int w = PiCapture::width;
	    int h4 = h/4;
	    memcpy(py->imageData,buffer->data,w*h);             // read Y
	    //unsigned char* pointer = (unsigned char *)(buffer -> data);
	    memcpy(pu->imageData,buffer->data+w*h,w*h4);        // read U
	    memcpy(pv->imageData,buffer->data+w*h+w*h4,w*h4);   // read V

	    cvResize(pu, pu_big, CV_INTER_NN);
	    cvResize(pv, pv_big, CV_INTER_NN);                  //CV_INTER_LINEAR looks better but it's slower
	    cvMerge(py, pu_big, pv_big, NULL, img);

	    cvCvtColor(img,dstImage,CV_YCrCb2RGB);              // convert in RGB color space (slow)
	    PiCapture::set_image(Mat(cvarrToMat(dstImage)));
	    mmal_buffer_header_mem_unlock(buffer);
	}
	else
	    cout << "Buffer NULL\n";
	
	mmal_buffer_header_release(buffer);
	if (port->is_enabled) {
		MMAL_STATUS_T status;
		new_buffer = mmal_queue_get(PiCapture::camera_video_port_pool->queue);
		if (new_buffer)
			status = mmal_port_send_buffer(port, new_buffer);
		if (!new_buffer || status != MMAL_SUCCESS)
			printf("Unable to return a buffer to the video port\n");
	}
}
static void gray_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer) {
    MMAL_BUFFER_HEADER_T *new_buffer;
    mmal_buffer_header_mem_lock(buffer);
    //unsigned char* pointer = (unsigned char *)(buffer -> data);
    int h = PiCapture::height;
    int w = PiCapture::width;
    memcpy(py->imageData,buffer->data,w*h);	// read Y
    PiCapture::set_image(Mat(cvarrToMat(py)));
    mmal_buffer_header_release(buffer);
    if (port->is_enabled) {
        MMAL_STATUS_T status;
        new_buffer = mmal_queue_get(PiCapture::camera_video_port_pool->queue);
        if (new_buffer)
            status = mmal_port_send_buffer(port, new_buffer);
        if (!new_buffer || status != MMAL_SUCCESS)
            printf("Unable to return a buffer to the video port\n");
    }
}
PiCapture::PiCapture(){
    
}
PiCapture::PiCapture(int _w, int _h, bool _color) {
    open(_w,_h,_color);
}
void PiCapture::open(int _w, int _h, bool _color) {
	
	py = cvCreateImage(cvSize(_w,_h), IPL_DEPTH_8U, 1);         // Y component of YUV I420 frame
	pu = cvCreateImage(cvSize(_w/2,_h/2), IPL_DEPTH_8U, 1);     // U component of YUV I420 frame
	pv = cvCreateImage(cvSize(_w/2,_h/2), IPL_DEPTH_8U, 1);     // V component of YUV I420 frame
	pu_big = cvCreateImage(cvSize(_w,_h), IPL_DEPTH_8U, 1);
	pv_big = cvCreateImage(cvSize(_w,_h), IPL_DEPTH_8U, 1);
	dstImage = cvCreateImage(cvSize(_w,_h), IPL_DEPTH_8U, 3);
	img = cvCreateImage(cvSize(_w,_h), IPL_DEPTH_8U, 3);        // final picture to display
	color = _color;
	width = _w;
	height = _h;
	camera = 0;
	preview = 0;
	camera_preview_port = NULL;
	camera_video_port = NULL;
	camera_still_port = NULL;
	preview_input_port = NULL;
	camera_preview_connection = 0;
	bcm_host_init();
	status = mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA, &camera);
	if (status != MMAL_SUCCESS) {
		printf("Error: create camera %x\n", status);
	}
	camera_preview_port = camera->output[MMAL_CAMERA_PREVIEW_PORT];
	camera_video_port = camera->output[MMAL_CAMERA_VIDEO_PORT];
	camera_still_port = camera->output[MMAL_CAMERA_CAPTURE_PORT];
	{
		MMAL_PARAMETER_CAMERA_CONFIG_T cam_config = {{ MMAL_PARAMETER_CAMERA_CONFIG, sizeof (cam_config)}, width, height, 0, 0,width, height, 3, 0, 1, MMAL_PARAM_TIMESTAMP_MODE_RESET_STC };
		mmal_port_parameter_set(camera->control, &cam_config.hdr);
	}
	
	
	//  set up the camera configuration
	{
	   MMAL_PARAMETER_CAMERA_CONFIG_T cam_config =
	   {
	      { MMAL_PARAMETER_CAMERA_CONFIG, sizeof(cam_config) },
	      cam_config.max_stills_w = width,
	      cam_config.max_stills_h = height,
	      cam_config.stills_yuv422 = 0,
	      cam_config.one_shot_stills = 0,
	      cam_config.max_preview_video_w = width,
	      cam_config.max_preview_video_h = height,
	      cam_config.num_preview_video_frames = 3,
	      cam_config.stills_capture_circular_buffer_height = 0,
	      cam_config.fast_preview_resume = 0,
	      cam_config.use_stc_timestamp = MMAL_PARAM_TIMESTAMP_MODE_RESET_STC
	   };
	   mmal_port_parameter_set(camera->control, &cam_config.hdr);
	}
	
	// Set the encode format on the video  port
	format = camera_video_port->format;
	format->encoding = MMAL_ENCODING_I420;
	format->encoding_variant = MMAL_ENCODING_I420;
	format->es->video.width = width;
	format->es->video.height = height;
	format->es->video.crop.x = 0;
	format->es->video.crop.y = 0;
	format->es->video.crop.width = width;
	format->es->video.crop.height = height;
	format->es->video.frame_rate.num = VIDEO_FRAME_RATE_NUM;
	format->es->video.frame_rate.den = VIDEO_FRAME_RATE_DEN;
	//camera_video_port->buffer_size =  width * height * 3 / 2;

	// Ensure there are enough buffers to avoid dropping frames
	if (camera_video_port->buffer_num < VIDEO_OUTPUT_BUFFERS_NUM)
	   camera_video_port->buffer_num = VIDEO_OUTPUT_BUFFERS_NUM;
	
	status = mmal_port_format_commit(camera_video_port);
	if (status != MMAL_SUCCESS) {
		printf("Error: unable to commit camera video port format (%u)\n", status);
	}
	
	
	// create pool form camera video port
	camera_video_port->buffer_size = camera_video_port->buffer_size_recommended;
	camera_video_port->buffer_num = camera_video_port->buffer_num_recommended;
	camera_video_port_pool = (MMAL_POOL_T *) mmal_port_pool_create(camera_video_port, camera_video_port->buffer_num, camera_video_port->buffer_size);
	
	if(color) {
		status = mmal_port_enable(camera_video_port, color_callback);
		if (status != MMAL_SUCCESS)
			printf("Error: unable to enable camera video port (%u)\n", status);
		else
			cout << "Attached color callback" << endl;
	}
	else {
		status = mmal_port_enable(camera_video_port, gray_callback);
		if (status != MMAL_SUCCESS)
			printf("Error: unable to enable camera video port (%u)\n", status);
		else
			cout << "Attached gray callback" << endl;
	}
	status = mmal_component_enable(camera);
	// Send all the buffers to the camera video port
	int num = mmal_queue_length(camera_video_port_pool->queue);
	int q;
	for (q = 0; q < num; q++) {
		MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(camera_video_port_pool->queue);
		if (!buffer) {
			printf("Unable to get a required buffer %d from pool queue\n", q);
		}
		if (mmal_port_send_buffer(camera_video_port, buffer) != MMAL_SUCCESS) {
			printf("Unable to send a buffer to encoder output port (%d)\n", q);
		}
	}
	if (mmal_port_parameter_set_boolean(camera_video_port, MMAL_PARAMETER_CAPTURE, 1) != MMAL_SUCCESS) {
		printf("%s: Failed to start capture\n", __func__);
	}
	cout << "Capture started" << endl;
}
//settings
int PiCapture::setSaturation(int saturation){
    int ret = 0;

       if (!camera)
          return 1;

       if (saturation >= -100 && saturation <= 100)
       {
          MMAL_RATIONAL_T value = {saturation, 100};
          ret = mmal_status_to_int(mmal_port_parameter_set_rational(camera->control, MMAL_PARAMETER_SATURATION, value));
       }
       else
       {
          printf("Invalid saturation value");
          ret = 1;
       }

       return ret;
}
int PiCapture::setSharpness(int sharpness){
    int ret = 0;

       if (!camera)
          return 1;

       if (sharpness >= -100 && sharpness <= 100)
       {
          MMAL_RATIONAL_T value = {sharpness, 100};
          ret = mmal_status_to_int(mmal_port_parameter_set_rational(camera->control, MMAL_PARAMETER_SHARPNESS, value));
       }
       else
       {
          printf("Invalid sharpness value");
          ret = 1;
       }

       return ret;
}
int PiCapture::setContrast(int contrast){
    int ret = 0;

      if (!camera)
         return 1;

      if (contrast >= -100 && contrast <= 100)
      {
         MMAL_RATIONAL_T value = {contrast, 100};
         ret = mmal_status_to_int(mmal_port_parameter_set_rational(camera->control, MMAL_PARAMETER_CONTRAST, value));
      }
      else
      {
         printf("Invalid contrast value");
         ret = 1;
      }

      return ret;
}
int PiCapture::setBrightness(int brightness){
    int ret = 0;

      if (!camera)
         return 1;

      if (brightness >= 0 && brightness <= 100)
      {
         MMAL_RATIONAL_T value = {brightness, 100};
         ret = mmal_status_to_int(mmal_port_parameter_set_rational(camera->control, MMAL_PARAMETER_BRIGHTNESS, value));
      }
      else
      {
         printf("Invalid brightness value");
         ret = 1;
      }

      return ret;
}
int PiCapture::setISO(int ISO){
    if (!camera)
         return 1;

      return mmal_status_to_int(mmal_port_parameter_set_uint32(camera->control, MMAL_PARAMETER_ISO, ISO));
}
int PiCapture::setExposureMeteringMode(MMAL_PARAM_EXPOSUREMETERINGMODE_T m_mode){
    MMAL_PARAMETER_EXPOSUREMETERINGMODE_T meter_mode = {{MMAL_PARAMETER_EXP_METERING_MODE,sizeof(meter_mode)},
                                                          m_mode};
       if (!camera)
          return 1;

       return mmal_status_to_int(mmal_port_parameter_set(camera->control, &meter_mode.hdr));
}
int PiCapture::setVideoStabilisation(int vstabilisation){
    if (!camera)
          return 1;

       return mmal_status_to_int(mmal_port_parameter_set_boolean(camera->control, MMAL_PARAMETER_VIDEO_STABILISATION, vstabilisation));
}
int PiCapture::setExposureCompensation(int exp_comp){
    if (!camera)
          return 1;

       return mmal_status_to_int(mmal_port_parameter_set_int32(camera->control, MMAL_PARAMETER_EXPOSURE_COMP , exp_comp));
}
int PiCapture::setExposureMode(MMAL_PARAM_EXPOSUREMODE_T mode){
    MMAL_PARAMETER_EXPOSUREMODE_T exp_mode = {{MMAL_PARAMETER_EXPOSURE_MODE,sizeof(exp_mode)}, mode};

       if (!camera)
          return 1;

       return mmal_status_to_int(mmal_port_parameter_set(camera->control, &exp_mode.hdr));
}
int PiCapture::setAWBMode(MMAL_PARAM_AWBMODE_T awb_mode){

    MMAL_PARAMETER_AWBMODE_T param = {{MMAL_PARAMETER_AWB_MODE,sizeof(param)}, awb_mode};

    if (!camera)
       return 1;

    return mmal_status_to_int(mmal_port_parameter_set(camera->control, &param.hdr));
}
int PiCapture::setAWBGains(float r_gain, float b_gain){
    MMAL_PARAMETER_AWB_GAINS_T param = {{MMAL_PARAMETER_CUSTOM_AWB_GAINS,sizeof(param)}, {0,0}, {0,0}};

      if (!camera)
         return 1;

      if (!r_gain || !b_gain)
         return 0;

      param.r_gain.num = (unsigned int)(r_gain * 65536);
      param.b_gain.num = (unsigned int)(b_gain * 65536);
      param.r_gain.den = param.b_gain.den = 65536;
      return mmal_status_to_int(mmal_port_parameter_set(camera->control, &param.hdr));
}
int PiCapture::setImageFX(MMAL_PARAM_IMAGEFX_T imageFX){
    MMAL_PARAMETER_IMAGEFX_T imgFX = {{MMAL_PARAMETER_IMAGE_EFFECT,sizeof(imgFX)}, imageFX};

      if (!camera)
         return 1;

      return mmal_status_to_int(mmal_port_parameter_set(camera->control, &imgFX.hdr));
}
int PiCapture::setColourFX(MMAL_PARAM_COLOURFX_T *colourFX){
    MMAL_PARAMETER_COLOURFX_T colfx = {{MMAL_PARAMETER_COLOUR_EFFECT,sizeof(colfx)}, 0, 0, 0};

      if (!camera)
         return 1;

      colfx.enable = colourFX->enable;
      colfx.u = colourFX->u;
      colfx.v = colourFX->v;

      return mmal_status_to_int(mmal_port_parameter_set(camera->control, &colfx.hdr));
}
int PiCapture::setRotation(int rotation){
    int ret;
       int my_rotation = ((rotation % 360 ) / 90) * 90;

       ret = mmal_port_parameter_set_int32(camera->output[0], MMAL_PARAMETER_ROTATION, my_rotation);
       mmal_port_parameter_set_int32(camera->output[1], MMAL_PARAMETER_ROTATION, my_rotation);
       mmal_port_parameter_set_int32(camera->output[2], MMAL_PARAMETER_ROTATION, my_rotation);

       return ret;
}
int PiCapture::setFlips(int hflip, int vflip){
    MMAL_PARAMETER_MIRROR_T mirror = {{MMAL_PARAMETER_MIRROR, sizeof(MMAL_PARAMETER_MIRROR_T)}, MMAL_PARAM_MIRROR_NONE};

      if (hflip && vflip)
         mirror.value = MMAL_PARAM_MIRROR_BOTH;
      else
      if (hflip)
         mirror.value = MMAL_PARAM_MIRROR_HORIZONTAL;
      else
      if (vflip)
         mirror.value = MMAL_PARAM_MIRROR_VERTICAL;

      mmal_port_parameter_set(camera->output[0], &mirror.hdr);
      mmal_port_parameter_set(camera->output[1], &mirror.hdr);
      return mmal_port_parameter_set(camera->output[2], &mirror.hdr);
}
int PiCapture::setROI(PARAM_FLOAT_RECT_T rect){
    MMAL_PARAMETER_INPUT_CROP_T crop = {{MMAL_PARAMETER_INPUT_CROP, sizeof(MMAL_PARAMETER_INPUT_CROP_T)}};
    if(rect.x < 0) rect.x = 0;if(rect.x > 1) rect.x = 1;
    if(rect.y < 0) rect.y = 0;if(rect.y > 1) rect.y = 1;
    if(rect.w < 0) rect.w = 0;if(rect.w > 1) rect.w = 1;
    if(rect.h < 0) rect.h= 0;if(rect.h > 1) rect.h = 1;
    crop.rect.x = (65536 * rect.x);
    crop.rect.y = (65536 * rect.y);
    crop.rect.width = (65536 * rect.w);
    crop.rect.height = (65536 * rect.h);

       return mmal_port_parameter_set(camera->control, &crop.hdr);
}
int PiCapture::setShutterSpeed(int speed){
    if (!camera)
          return 1;

       return mmal_status_to_int(mmal_port_parameter_set_uint32(camera->control, MMAL_PARAMETER_SHUTTER_SPEED, speed));
}
