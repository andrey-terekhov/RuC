void main()
{
    int message;
    
    wifi_connect("str1", "str2");
    blynk_terminal(11, "str3");
    blynk_authorization("str4");
    blynk_notification("str5");
    blynk_send(22, 33);
    message = blynk_receive(44);
    blynk_property(66, "color", "#000000");
    blynk_lcd(77, 88, 99, "str8");
}