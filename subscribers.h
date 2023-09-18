#ifdef SUBSCRIBERS_H_
#define SUBSCRIBERS_H_


void register_id(int sockfd, char* id);
void subscribe(int sockfd, char* topic, int SF);
void unsubscribe(int sockfd, char* topic);

#endif // SUBSCRIBERS_H_