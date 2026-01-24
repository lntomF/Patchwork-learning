//The first 
void LED_Task(void *p, void *time)
{
    ledconfig_t *config = (ledconfig_t *)p;
    int *delay_time = (int *)time;
    printf("%d",*delay_time);
    printf("操作硬件：端口 %d, 引脚 %d\n", config->port_num, config->pin_num);
}
//The second
int A=1,B=2;
OS_Start_Task(Read_Sensor,&A);
OS_Start_Task(Read_Sensor,&B);
void Read_Sensor(void *p)
{
    int *ID = (int *)p;
    printf("The ID is %d",*ID);
}