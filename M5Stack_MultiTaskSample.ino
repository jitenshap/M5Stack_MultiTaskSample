#include <M5Stack.h>

TaskHandle_t nyan_task_handle;
EventGroupHandle_t lcd_event_group;
SemaphoreHandle_t global_mutex = NULL;
QueueHandle_t lcd_queue;

int NYAN_END_BIT = BIT0;

typedef struct lcd_update_t
{
  bool cls;
  int x;
  int y;
  char msg[16];
}lcd_update_t;

long count = 0;


void unlock_global()
{
  xSemaphoreGive(global_mutex);
}

bool lock_global(TickType_t xTicksToWait)
{
  if(global_mutex)
  {
    if(xSemaphoreTake(global_mutex, xTicksToWait) == pdTRUE)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
  else
  {
    return false;
  }
}

void slow_task(void *pvParameters)
{
  char * message = (char *)pvParameters;
  for(int i = 0; i < 5; i ++)
  {
    lcd_update_t payload;
    if(i == 0)
      payload.cls= true;
    else
      payload.cls = false;
    payload.x = 0;
    payload.y = i * 40;
    strcpy(payload.msg, message);
    xQueueSend(lcd_queue, &payload, 5000 / portTICK_PERIOD_MS);
    delay(1000);
  }
  xEventGroupSetBits(lcd_event_group, NYAN_END_BIT);  
  vTaskDelete(NULL);
}

void core0_task(void *pvParameters)
{
  while(true)
  {
    delay(1000);
    xEventGroupClearBits(lcd_event_group, NYAN_END_BIT);
    char message[16];
    sprintf(message, "nya-n");
    xTaskCreatePinnedToCore(&slow_task, "slow_task", 1024, message, 3, &nyan_task_handle, 1);
    EventBits_t uxBits = xEventGroupWaitBits(lcd_event_group, NYAN_END_BIT, pdFALSE, pdTRUE, 10000 / portTICK_PERIOD_MS);
    if(uxBits & NYAN_END_BIT)
    {
      Serial.println("slowTask end");
    }
    else
    {
      Serial.println("slowTask timed out");
      if(eTaskGetState(nyan_task_handle) == eRunning)
      {
        vTaskDelete(nyan_task_handle);
      }      
    }    
    lcd_update_t payload;
    payload.cls = true;
    payload.x = 0;
    payload.y = 0;
    if (lock_global(1000 / portTICK_PERIOD_MS))
    {
      sprintf(payload.msg, "Count = %ld", count);
      count ++;    
    }
    unlock_global();
    xQueueSend(lcd_queue, &payload, 5000 / portTICK_PERIOD_MS); 
  }
}


// the setup routine runs once when M5Stack starts up
void setup()
{
  // Initialize the M5Stack object
  M5.begin();
  M5.Power.begin();
  M5.Speaker.begin();
  M5.Lcd.setTextSize(2);
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.setTextFont(4);
  M5.Lcd.print("Hello");
  lcd_event_group = xEventGroupCreate();
  global_mutex = xSemaphoreCreateMutex();
  lcd_queue = xQueueCreate( 1, sizeof(lcd_update_t));
  xTaskCreatePinnedToCore(core1_task, "core1_task", 8192, NULL, 10, NULL, 1);
  xTaskCreatePinnedToCore(core0_task, "core0_task", 4096, NULL, 5, NULL, 0);
  double pi = 0;
}

// the loop routine runs over and over again forever

void core1_task(void *pvParameters)
{
  while(true)
  {  
    lcd_update_t lcdbuf;
    if( xQueueReceive(lcd_queue, &lcdbuf, (TickType_t)1) == pdTRUE)
    {
      Serial.println("Got queue");
      M5.Lcd.setCursor(lcdbuf.x, lcdbuf.y);
      if(lcdbuf.cls)
        M5.Lcd.fillScreen(TFT_BLACK);
      M5.Lcd.print(lcdbuf.msg);
    }  
  }
}

void loop()
{
}
