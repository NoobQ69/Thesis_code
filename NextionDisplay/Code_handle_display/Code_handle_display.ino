void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial2.begin(9600);

  Serial.println("Pass");
}

void handle_recieved_data_serial()
{
  if (Serial2.available())
  {
    String data_str = "";
    delay(10);
    while (Serial2.available())
    {
      data_str += char(Serial.read());
    }
    Serial.println(data_str);
  }
}

void handle_value_from_lcd(String value_str)
{
  if (value_str.startsWith("home"))
  {
    Serial.println("Go home");
    return;
  }
}

// Progress bar

void handle_progress_bar()
{
  int i = 0;
  Serial2.print("j0.val="+String(i));
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  i++;
  delay(50);
}

void loop() {
  // put your main code here, to run repeatedly:
  handle_recieved_data_serial();
  delay(20);
}
