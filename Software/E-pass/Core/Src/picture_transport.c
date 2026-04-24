#include "picture_transport.h"

#include "fatfs.h"
#include "main.h"
#include "usbd_cdc_if.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define PT_LINE_BUF_SIZE      128U
#define PT_RESP_QUEUE_DEPTH   8U
#define PT_RESP_TEXT_MAX      96U

typedef struct
{
  uint8_t active;
  uint32_t expected;
  uint32_t received;
  FIL file;
  char name[64];
} pt_file_session_t;

static uint8_t g_line_buf[PT_LINE_BUF_SIZE];
static uint32_t g_line_len;

static char g_resp_queue[PT_RESP_QUEUE_DEPTH][PT_RESP_TEXT_MAX];
static volatile uint32_t g_resp_head;
static volatile uint32_t g_resp_tail;
static volatile uint32_t g_resp_count;

static pt_file_session_t g_file;
static uint8_t g_fs_mounted;

static void pt_queue_response(const char *msg)
{
  __disable_irq();
  if (g_resp_count < PT_RESP_QUEUE_DEPTH)
  {
    (void)strncpy(g_resp_queue[g_resp_head], msg, PT_RESP_TEXT_MAX - 1U);
    g_resp_queue[g_resp_head][PT_RESP_TEXT_MAX - 1U] = '\0';
    g_resp_head = (g_resp_head + 1U) % PT_RESP_QUEUE_DEPTH;
    g_resp_count++;
  }
  __enable_irq();
}

static void pt_queue_response_fmt_u32(const char *fmt, uint32_t a, uint32_t b)
{
  char tmp[PT_RESP_TEXT_MAX];
  (void)snprintf(tmp, sizeof(tmp), fmt, (unsigned long)a, (unsigned long)b);
  pt_queue_response(tmp);
}

static uint8_t pt_parse_u32(const char *s, uint32_t *value)
{
  uint32_t out = 0U;

  if ((s == NULL) || (*s == '\0'))
  {
    return 0U;
  }

  while (*s != '\0')
  {
    if (!isdigit((unsigned char)*s))
    {
      return 0U;
    }
    out = (out * 10U) + (uint32_t)(*s - '0');
    s++;
  }

  *value = out;
  return 1U;
}

static uint8_t pt_is_filename_safe(const char *name)
{
  const char *p = name;

  if ((name == NULL) || (*name == '\0'))
  {
    return 0U;
  }

  while (*p != '\0')
  {
    if (!(isalnum((unsigned char)*p) || (*p == '_') || (*p == '-') || (*p == '.') ))
    {
      return 0U;
    }
    p++;
  }

  if (strstr(name, "..") != NULL)
  {
    return 0U;
  }

  return 1U;
}

static void pt_close_active_file(void)
{
  if (g_file.active != 0U)
  {
    (void)f_close(&g_file.file);
  }

  g_file.active = 0U;
  g_file.expected = 0U;
  g_file.received = 0U;
  g_file.name[0] = '\0';
}

static uint8_t pt_mount_fs(void)
{
  FRESULT fr;

  if (g_fs_mounted != 0U)
  {
    return 1U;
  }

  fr = f_mount(&SDFatFS, (TCHAR const*)SDPath, 1U);
  if (fr == FR_OK)
  {
    g_fs_mounted = 1U;
    return 1U;
  }

  return 0U;
}

static void pt_handle_command(char *line)
{
  char *cmd = strtok(line, " ");

  if (cmd == NULL)
  {
    return;
  }

  if (strcmp(cmd, "PING") == 0)
  {
    pt_queue_response("PONG\r\n");
    return;
  }

  if (strcmp(cmd, "HELP") == 0)
  {
    pt_queue_response("CMD: PING, HELP, MOUNT, START <name> <bytes>, ABORT\r\n");
    return;
  }

  if (strcmp(cmd, "MOUNT") == 0)
  {
    if (pt_mount_fs() != 0U)
    {
      pt_queue_response("OK MOUNT\r\n");
    }
    else
    {
      pt_queue_response("ERR MOUNT\r\n");
    }
    return;
  }

  if (strcmp(cmd, "ABORT") == 0)
  {
    pt_close_active_file();
    pt_queue_response("OK ABORT\r\n");
    return;
  }

  if (strcmp(cmd, "START") == 0)
  {
    char *name = strtok(NULL, " ");
    char *size_s = strtok(NULL, " ");
    uint32_t size_bytes;
    char path[96];
    FRESULT fr;

    if ((pt_is_filename_safe(name) == 0U) || (pt_parse_u32(size_s, &size_bytes) == 0U) || (size_bytes == 0U))
    {
      pt_queue_response("ERR ARGS\r\n");
      return;
    }

    if (pt_mount_fs() == 0U)
    {
      pt_queue_response("ERR MOUNT\r\n");
      return;
    }

    if (g_file.active != 0U)
    {
      pt_queue_response("ERR BUSY\r\n");
      return;
    }

    fr = f_mkdir("0:/images");
    if ((fr != FR_OK) && (fr != FR_EXIST))
    {
      pt_queue_response("ERR MKDIR\r\n");
      return;
    }

    (void)snprintf(path, sizeof(path), "0:/images/%s", name);

    fr = f_open(&g_file.file, path, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK)
    {
      pt_queue_response("ERR OPEN\r\n");
      return;
    }

    g_file.active = 1U;
    g_file.expected = size_bytes;
    g_file.received = 0U;
    (void)strncpy(g_file.name, name, sizeof(g_file.name) - 1U);
    g_file.name[sizeof(g_file.name) - 1U] = '\0';

    pt_queue_response_fmt_u32("READY %lu\r\n", size_bytes, 0U);
    return;
  }

  pt_queue_response("ERR CMD\r\n");
}

void PictureTransport_Init(void)
{
  g_line_len = 0U;

  g_resp_head = 0U;
  g_resp_tail = 0U;
  g_resp_count = 0U;

  g_fs_mounted = 0U;
  g_file.active = 0U;
  g_file.expected = 0U;
  g_file.received = 0U;
  g_file.name[0] = '\0';
}

void PictureTransport_OnUsbRx(const uint8_t *data, uint32_t len)
{
  uint32_t i;

  if ((data == NULL) || (len == 0U))
  {
    return;
  }

  if (g_file.active != 0U)
  {
    UINT bw = 0U;
    uint32_t remaining = g_file.expected - g_file.received;
    uint32_t to_write = (len < remaining) ? len : remaining;
    FRESULT fr;

    fr = f_write(&g_file.file, data, (UINT)to_write, &bw);
    if ((fr != FR_OK) || (bw != (UINT)to_write))
    {
      pt_close_active_file();
      pt_queue_response("ERR WRITE\r\n");
      return;
    }

    g_file.received += to_write;

    if (g_file.received == g_file.expected)
    {
      (void)f_sync(&g_file.file);
      pt_close_active_file();
      pt_queue_response("OK WRITE\r\n");
      return;
    }

    if (to_write < len)
    {
      pt_close_active_file();
      pt_queue_response("ERR EXTRA\r\n");
      return;
    }

    return;
  }

  for (i = 0U; i < len; i++)
  {
    uint8_t c = data[i];

    if ((c == '\r') || (c == '\n'))
    {
      if (g_line_len > 0U)
      {
        char line[PT_LINE_BUF_SIZE];
        memcpy(line, g_line_buf, g_line_len);
        line[g_line_len] = '\0';
        g_line_len = 0U;
        pt_handle_command(line);
      }
      continue;
    }

    if (g_line_len < (PT_LINE_BUF_SIZE - 1U))
    {
      g_line_buf[g_line_len++] = c;
    }
    else
    {
      g_line_len = 0U;
      pt_queue_response("ERR LINE\r\n");
    }
  }
}

void PictureTransport_Task(void)
{
  uint8_t tx_result;

  if (g_resp_count == 0U)
  {
    return;
  }

  tx_result = CDC_Transmit_FS((uint8_t *)g_resp_queue[g_resp_tail], (uint16_t)strlen(g_resp_queue[g_resp_tail]));
  if (tx_result == USBD_OK)
  {
    __disable_irq();
    if (g_resp_count > 0U)
    {
      g_resp_tail = (g_resp_tail + 1U) % PT_RESP_QUEUE_DEPTH;
      g_resp_count--;
    }
    __enable_irq();
  }
}
