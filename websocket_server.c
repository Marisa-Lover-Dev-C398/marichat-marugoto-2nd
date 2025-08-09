#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <libwebsockets.h>

#define ROOM_MAX 10
#define MAX_USERS 32
#define MAX_PAYLOAD 512

enum user_status {
  USER_STATUS_OFFLINE,
  USER_STATUS_ONLINE,
};

enum send_status {
  SEND_STATUS_UNSENT,
  SEND_STATUS_FAILED,
  SEND_STATUS_SENT,
};

struct send_request {
  char msg[1024];
  size_t len;
  bool unsent;
};
struct user_data{
  struct lws *wsi;
  int uid;
  char* name;
  int room_id;
  enum user_status u_status; 
  enum send_status s_status;
  struct send_request req;
};

static int people = 0;
static struct user_data* users[MAX_USERS];
static int callback(struct lws *wsi, enum lws_callback_reasons reason,
                    void *user, void *in, size_t len) {
  struct user_data *data = (struct user_data*) user;
  switch(reason)
  {
    case LWS_CALLBACK_ESTABLISHED:
      printf("[line:3(int)callback()]接続来たよ^^\n\n");
      for(int i = 0;i < MAX_USERS; i++)
      {
        if(users[i]==NULL){
          users[i] = data;
          data->wsi = wsi;
          data->uid = i;
          data->room_id = 1;
          printf("==>ユーザー情報\n==>登録位置:%d\n==>UID:%d\n",i,data->uid);
          break;
        }
      }
      printf("==>wsソケット情報を保存しました。\n");
    break;
    case LWS_CALLBACK_RECEIVE:
      if((int)len < 1) return 1;
      printf("\n\nメッセージ受信: %.*s\n", (int)len, (const char *)in);
      bool found = false;
      int uid = 0;
      for (uid= 0; uid < MAX_USERS; uid++) {
        if(users[uid]==(struct user_data*) user)
        {
          printf("==> Found-UID:%d,Data's-UID:%d",uid,users[uid]->uid);
          found = true;
          break;
        }
      }
      if(!found)  return 1;
      for(int i = 0; i < MAX_USERS; i++)
      {
        if(users[i] && users[i]->room_id == users[uid]->room_id){
          strcpy(users[i]->req.msg, (const char *)in);
          users[i]->req.len = len;
          users[i]->req.unsent = true;
          lws_callback_on_writable(users[i]->wsi);
        }
      }
    break;
    case LWS_CALLBACK_CLOSED:
      for (int i = 0; i < MAX_USERS; ++i) {
        if (users[i] == user) {
           users[i] = NULL;
           printf("\nUID:%dさんが切断しました。\n",i);
           break;
        }
      }
    break;
    case LWS_CALLBACK_SERVER_WRITEABLE:
      printf("\n[line:3(int)callback()]今からクライアントに送信するよ〜\n");
      if(users[data->uid] && data->req.unsent){
        char buf[users[data->uid]->req.len + MAX_PAYLOAD];
        int n;
        memset(buf, 0, sizeof(buf));
       // memcpy(&buf[users[data->uid]->req.len], users[data->uid]->req.msg, users[data->uid]->req.len);
        memcpy(buf + LWS_PRE, users[data->uid]->req.msg, users[data->uid]->req.len);
        n = lws_write(wsi, (unsigned char *)buf + LWS_PRE, users[data->uid]->req.len, LWS_WRITE_TEXT);
        //n = lws_write(wsi, (unsigned char*)&buf[users[data->uid]->req.len], users[data->uid]->req.len, LWS_WRITE_TEXT);
        if (n < 0) {
          lwsl_err("send failed\n");
          return -1;
        }
        data->req.unsent = false;
      }
    break;

  }
  return 0;
}

static struct lws_protocols protocols[] = {
  {
      .name = "marisa-server3-test",
      .callback = callback,
      .per_session_data_size = sizeof(struct user_data),
  },
  { NULL, NULL, 0 }
};

int main(void)
{
  struct lws_context_creation_info info;
  struct lws_context *context;

  memset(&info, 0, sizeof info);
  info.port = 7070;
  info.protocols = protocols;
  context = lws_create_context(&info);
  if(!context) return 1;
  while(1)
  {
    lws_service(context, 6000);
  }
  lws_context_destroy(context);
  return 0;
}
