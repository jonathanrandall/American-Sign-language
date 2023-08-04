// #include "Arduino.h"
#include <TensorFlowLite_ESP32.h>

#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"

#include "p_det_model.h"
#include "model_settings.h"

#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include <esp_heap_caps.h>

#include "esp_camera.h"

#include "img_converters.h"
#include "Free_Fonts.h"

#include "soc/soc.h" // Disable brownout problems
#include "soc/rtc_cntl_reg.h" // Disable brownout problems

// Select camera model
// #define CAMERA_MODEL_WROVER_KIT // Has PSRAM
//#define CAMERA_MODEL_ESP_EYE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE  // Has PSRAM
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM

#include "camera_pins.h"
#include "downsample.h"
#include "tft_stuff.h"

// #include <fb_gfx.h>


// #include <TJpg_Decoder.h>
// #include <SPI.h>
// #include <TFT_eSPI.h>
// TFT_eSPI tft = TFT_eSPI();
// #include <TFT_eFEX.h>
// TFT_eFEX  fex = TFT_eFEX(&tft);
#define TEXT "starting app..."

camera_fb_t * fb = NULL;
uint16_t *buffer;

size_t _jpg_buf_len = 0;
uint8_t * _jpg_buf = NULL;


//tflite stuff
tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;

// In order to use optimized tensorflow lite kernels, a signed int8_t quantized
// model is preferred over the legacy unsigned model format. This means that
// throughout this project, input images must be converted from unisgned to
// signed format. The easiest and quickest way to convert from unsigned to
// signed 8-bit integers is to subtract 128 from the unsigned value to get a
// signed value.

#ifdef CONFIG_IDF_TARGET_ESP32S3
constexpr int scratchBufSize = 39 * 1024;
#else
constexpr int scratchBufSize = 0;
#endif
// An area of memory to use for input, output, and intermediate arrays.
constexpr int kTensorArenaSize = 81 * 1024 + scratchBufSize;
static uint8_t *tensor_arena;//[kTensorArenaSize]; // Maybe we should move this to external

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
   // Stop further decoding as image is running off bottom of screen
  if ( y >= tft.height() ) return 0;

  // This function will clip the image block rendering automatically at the TFT boundaries
  tft.pushImage(x, y, w, h, bitmap);

  // This might work instead if you adapt the sketch to use the Adafruit_GFX library
  // tft.drawRGBBitmap(x, y, bitmap, w, h);

  // Return 1 to decode next block
  return 1;
}

void init_camera(){
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_RGB565;//PIXFORMAT_GRAYSCALE;//PIXFORMAT_JPEG;//PIXFORMAT_RGB565;// 
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.frame_size = FRAMESIZE_QVGA;//FRAMESIZE_96X96;//FRAMESIZE_QVGA;//FRAMESIZE_96X96;//
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(psramFound()){
    // config.frame_size = FRAMESIZE_QVGA;//FRAMESIZE_96X96;//
    config.jpeg_quality = 12;
    config.fb_count = 2;
  } else {
    // config.frame_size = FRAMESIZE_QVGA;//FRAMESIZE_96X96;//
    config.jpeg_quality = 12;
    config.fb_count = 2;
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  }
#if defined(CAMERA_MODEL_M5STACK_WIDE)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif
}


void setup() {
  
  // put your setup code here, to run once:
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);//disable brownout detector

  Serial.begin(115200);
  init_camera();
  // buffer = (uint16_t *) malloc(240*320*2);

  SPI.begin(TFT_SCLK,TFT_MISO,TFT_MOSI,TFT_CS);
  tft.begin();
  tft.setRotation(1);  // 0 & 2 Portrait. 1 & 3 landscape
  tft.fillScreen(TFT_BLACK);
  SPI.begin(TFT_SCLK,TFT_MISO,TFT_MOSI,TFT_CS);
  tft.begin();
  tft.setRotation(3);  // 0 & 2 Portrait. 1 & 3 landscape
  tft.fillScreen(TFT_BLACK);
  

  // Set text colour to orange with black background
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);

  tft.setFreeFont(FF1);                 // Select the font
  tft.drawString(TEXT, 160, 120, GFXFF);// Print the string name of the font
//  tft.drawString(String(WiFi.localIP()).c_str(), 160, 180, GFXFF);
  tft.setCursor(50, 180, 2);
  
  tft.setTextColor(TFT_GREEN, TFT_BLACK);    tft.setTextFont(4);

  // //The jpeg image can be scaled by a factor of 1, 2, 4, or 8
  TJpgDec.setJpgScale(1);

  // The byte order can be swapped (set true for TFT_eSPI)
  TJpgDec.setSwapBytes(true);

  // The decoder must be given the exact name of the rendering function above
  TJpgDec.setCallback(tft_output);
  dstImage = (uint16_t *) malloc(DST_WIDTH * DST_HEIGHT*2);
  delay(200);
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;

  model = tflite::GetModel(g_person_detect_model_data);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    TF_LITE_REPORT_ERROR(error_reporter,
                         "Model provided is schema version %d not equal "
                         "to supported version %d.",
                         model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }
  if (tensor_arena == NULL) {
    tensor_arena = (uint8_t *) heap_caps_malloc(kTensorArenaSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  }
  if (tensor_arena == NULL) {
    printf("Couldn't allocate memory of %d bytes\n", kTensorArenaSize);
    return;
  }

  // Pull in only the operation implementations we need.
  // This relies on a complete list of all the ops needed by this graph.
  // An easier approach is to just use the AllOpsResolver, but this will
  // incur some penalty in code space for op implementations that are not
  // needed by this graph.
  //
  /*/
  tflite::AllOpsResolver resolver;
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
      *///
  // NOLINTNEXTLINE(runtime-global-variables)
  //
  static tflite::MicroMutableOpResolver<5> micro_op_resolver;
  micro_op_resolver.AddAveragePool2D();
  micro_op_resolver.AddConv2D();
  micro_op_resolver.AddDepthwiseConv2D();
  micro_op_resolver.AddReshape();
  micro_op_resolver.AddSoftmax();
  

  // Build an interpreter to run the model with.
  // NOLINTNEXTLINE(runtime-global-variables)
  static tflite::MicroInterpreter static_interpreter(
      model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);
      ////
  
  interpreter = &static_interpreter;

  // Allocate memory from the tensor_arena for the model's tensors.
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed");
    return;
  }

  // Get information about the memory area to use for the model's input.
  input = interpreter->input(0);

}

void loop() {
  // put your main code here, to run repeatedly:
  for (int i = 0; i<2; i++){
  fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
    }
    if(fb){
      esp_camera_fb_return(fb);
      fb=NULL;
    }
  delay(1);
  }
  fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
    }
  uint16_t * tmp = (uint16_t *) fb->buf;

    downsampleImage((uint16_t *) fb->buf, fb->width, fb->height);
    bool jpeg_converted = frame2jpg(fb, 90, &_jpg_buf, &_jpg_buf_len);

    for (int y = 0; y < DST_HEIGHT; y++) {
      for (int x = 0; x < DST_WIDTH; x++) {
        tmp[y*(fb->width) + x] = (uint16_t) dstImage[y*DST_WIDTH +x];

      }
    }
    
    // TJpgDec.drawJpg(0,0,(const uint8_t*) _jpg_buf, _jpg_buf_len);
    tft_output(0, 0, fb->width, fb->height, (uint16_t *) fb->buf);
    // tft_output(50, 50, 96, 96, dstImage);
    delay(10);
    // Serial.println((String(fb->len)).c_str());
    // Serial.println((String(sizeof(fb->buf))).c_str());
    delay(20);
     
    if(fb){
      esp_camera_fb_return(fb);
      if(_jpg_buf){
      free(_jpg_buf);}
      fb = NULL;
      _jpg_buf = NULL;
    }

    int8_t * image_data = input->data.int8;
    Serial.println(input->dims->size);

    for (int i = 0; i < kNumRows; i++) {
      for (int j = 0; j < kNumCols; j++) {
        uint16_t pixel = ((uint16_t *) (dstImage))[i * kNumCols + j];

        // for inference
        uint8_t hb = pixel & 0xFF;
        uint8_t lb = pixel >> 8;
        uint8_t r = (lb & 0x1F) << 3;
        uint8_t g = ((hb & 0x07) << 5) | ((lb & 0xE0) >> 3);
        uint8_t b = (hb & 0xF8);

        /**
        * Gamma corected rgb to greyscale formula: Y = 0.299R + 0.587G + 0.114B
        * for effiency we use some tricks on this + quantize to [-128, 127]
        */
        int8_t grey_pixel = ((305 * r + 600 * g + 119 * b) >> 10) - 128;

        image_data[i * kNumCols + j] = grey_pixel;

        // to display
        // display_buf[2 * i * kNumCols * 2 + 2 * j] = pixel;
        // display_buf[2 * i * kNumCols * 2 + 2 * j + 1] = pixel;
        // display_buf[(2 * i + 1) * kNumCols * 2 + 2 * j] = pixel;
        // display_buf[(2 * i + 1) * kNumCols * 2 + 2 * j + 1] = pixel;
      }
    }

    

  if (kTfLiteOk != interpreter->Invoke()) {
    TF_LITE_REPORT_ERROR(error_reporter, "Invoke failed.");
  }
  

  TfLiteTensor* output = interpreter->output(0);
  

  // Process the inference results.
  int8_t person_score = output->data.uint8[kPersonIndex];
  
  int8_t no_person_score = output->data.uint8[kNotAPersonIndex];
  
  float person_score_f =
      (person_score - output->params.zero_point) * output->params.scale;
  float no_person_score_f =
      (no_person_score - output->params.zero_point) * output->params.scale;

  Serial.print("person score: "); Serial.println(person_score_f);
  Serial.print("no person score: "); Serial.println(no_person_score_f);

  // tft.setTextSize(2); 
  // String fps = "avg fps " + String(avg_fps);
  // tft.drawString(fps, 40, 12);
  //fb_gfx_printf(fb, 40, 12, 0xffff, "avg fps:%3d", avg_fps);
        //tft.fillColor(color_det);
  if (person_score_f>no_person_score_f){
      tft.fillRect(0, 200, 40, 40,TFT_GREEN );
  } else {
    tft.fillRect(0, 200, 40, 40,TFT_RED );
  }
  
    // esp_camera_fb_return(fb);
    // fb = NULL;
    // free(buffer);
  delay(2000);

}
