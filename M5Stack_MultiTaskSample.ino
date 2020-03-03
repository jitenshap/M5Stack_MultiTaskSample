#include <M5Stack.h>

TaskHandle_t core0_task_handle;
EventGroupHandle_t core0_event_group;
SemaphoreHandle_t global_mutex = NULL;
QueueHandle_t lcd_queue;

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

void core0_task(void* arg)
{
  while(true)
  {
    delay(1000);
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
  core0_event_group = xEventGroupCreate();
  global_mutex = xSemaphoreCreateMutex();
  lcd_queue = xQueueCreate( 1, sizeof(lcd_update_t));
  xTaskCreatePinnedToCore(core0_task, "core0_task", 4096, NULL, 2, &core0_task_handle, 0);
}

// the loop routine runs over and over again forever
void loop() //Core1 task
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
