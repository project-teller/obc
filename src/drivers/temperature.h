namespace teller::drivers::temperature {

bool init(void);
void destroy(void);
bool setup(void);
bool update(float& temperature);

}
