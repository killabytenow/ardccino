// *** Hardwarespecific functions ***
void UTFT::_hw_special_init()
{
}

void UTFT::LCD_Writ_Bus(char VH,char VL, byte mode)
{   
#warning "escribir en pantalla VH:VL bytes"
}

void UTFT::_set_direction_registers(byte mode)
{
}

void UTFT::_fast_fill_16(int ch, int cl, long pix)
{
	// ch - color high
	// cl - color low
	// pix - number of pixels X*Y
}

void UTFT::_fast_fill_8(int ch, long pix)
{
	long blocks;

    	// ch - color high?
	// pix - number of pixels X*Y
}
