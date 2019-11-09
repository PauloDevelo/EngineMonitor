String FloatToString(float number, uint8_t digits)
{
  char buf[18];
  size_t n = 0;

  if (isnan(number)) strcpy(buf, "nan");
  else if (isinf(number)) strcpy(buf, "inf");
  else if (number > 4294967040.0) strcpy(buf, "ovf");  // constant determined empirically
  else if (number <-4294967040.0) strcpy(buf, "ovf");  // constant determined empirically
  else{
    // Handle negative numbers
    if (number < 0.0){
      buf[n++] = '-';
      number = -number;
    }

    // Round correctly so that print(1.999, 2) prints as "2.00"
    double rounding = 0.5;
    for (uint8_t i=0; i<digits; ++i)
      rounding /= 10.0;

    number += rounding;

    // Extract the integer part of the number and print it
    unsigned long int_part = (unsigned long)number;
    double remainder = number - (double)int_part;
    itoa(int_part, buf + n, 10);
    
    n++;
    while(int_part > 10){
      n++;
      int_part /= 10;
    }

    // Print the decimal point, but only if there are digits beyond
    if (digits > 0) {
      buf[n++] = '.';
    }

    // Extract digits from the remainder one at a time
    while (digits-- > 0)
    {
      remainder *= 10.0;
      int toPrint = int(remainder);
      itoa(toPrint, buf+n, 10);
      n++;
      remainder -= toPrint;
    }
    
    //Caractère de fin
    buf[n] = 0;
  }
  
  String str = String(buf);
  return str;
}

String lengthStr(String value, int lgth){
  value = value.substring(0, lgth);
  while(value.length() < lgth){
    value += " ";
  }
  return value;
}
