/*
 * main implementation: use this 'C' sample to create your own application
 *
 */


//#include "derivative.h" /* include peripheral declarations */
# include "TFC.h"
		
int print_files_menu(int file_index);
void receive_file(int index);
void send_file();


int main(void){

	char_idx = 0;
	dataready = 0;
	ClockSetup();
	InitGPIO();
	InitPIT();
	lcd_init();
	dma_init();
	InitUARTs(9600);
	RGB_LED_OFF;
	lcd_clear();
	UARTprintf(UART0_BASE_PTR,"\n");
	
	files_num = 0;
	index_last = -1;
	index_first = 0;
	last_addr = &files;
	state = IDLE_STATE_0;
	rfile_mode = 0x01;
	
	while(1){
	
		if (dataready)
		{ 
			
			dataready = 0;
			
			//---------------------------------------------------------------------------------------------------------------------
			// On-line update of Terminal Configuration
			// --------------------------------------------------------------------------------------------------------------------
			if (strncmp(PC_msg,"ConnectionParams", 16) == 0){
				int j;
				UARTprintf(UART0_BASE_PTR,"ack\n");
				for (j=10000; j>0; j--);
				UART0_C2 &= 0xF3;			   // stop bit only be changed when the transmitter(bit3) and receiver(bit2) are both disabled
				int baud = 0;
				int i = 0;
			
				switch (PC_msg[17]){    //Check baud rate
					case '9': 
						baud = 9600; 
						break;
					case '2': 
						baud = 2400; 
						break;
					case '1': 
						baud = 19200; 
						i+=1; 
						break;
					case '3': 
						baud = 38400; 
						i+=1; 
						break;
					default: 
						baud = 9600;
				}

 				InitUARTs(baud);
			
				switch (PC_msg[22+i]){   //Check parity bit
					case 'N': 			// Parity = none
						UART0_C1 &= 0xFD;   // UART0_C1.PE = 0
						i+=1 ; 
						break;                         
					case 'E': 			// Parity = even
						UART0_C1 |= 0x02; 	//UART0_C1.PE = 1
						UART0_C1 &= 0xFE;	//UART0_C1.PT = 0 
						i+=1 ; 
						break;          
					case 'O': 			// Parity = odd
						UART0_C1 |= 0x02; 	//UART0_C1.PE = 1
						UART0_C1 |= 0x01; 	//UART0_C1.PT = 1
						break;			      
					default: 
						UART0_C1 &= 0xFD; //default parity = none
				}
				
				switch (PC_msg[26+i]){	//Check stop bits									
				   case '1': 			// Stop bit = 1
					   UART0_BDH &= 0xDF; 	//UART0_BDH.SBNS = 0
					   break;							      	
				   case '2': 			// Stop bit = 2
					   UART0_BDH |= 0x20; 	//UART0_BDH.SBNS = 1
					   break;
				   default:
					   UART0_BDH &= 0xDF; //default Stop bit = 1
				}
				
				UART0_C2 = UARTLP_C2_RE_MASK | UARTLP_C2_TE_MASK | UART_C2_RIE_MASK; // Enable Transmitter, Receiver, Receive interrupt					                            // Call the function that set the baud rate
				memset(PC_msg,0,40);  //memset - clears the array
			}
			
			//---------------------------------------------------------------------------------------------------------------------
			// Chat mode
			//---------------------------------------------------------------------------------------------------------------------
			if ( strncmp(PC_msg,"msg", 3) == 0 ){// Modified from: if ((PC_msg[0] == 'm') & (PC_msg[1] == 's') & (PC_msg[2] == 'g') ){ //message received 
				lcd_clear();               
				int i = 4;
				int j;
				while (PC_msg[i] != '\r')
				{
					for (j=10000; j>0; j--);	 	  // Delay
					if (i==20){
						lcd_new_line();        	  //second line
						for (j=10000; j>0; j--);	  // Delay
					}
					lcd_data(PC_msg[i]);           //print char
					i+=1;
				}
				memset(PC_msg,0,40);               //memset - clears the array
			}
			
			//---------------------------------------------------------------------------------------------------------------------
			// Receive Files from PC
			//---------------------------------------------------------------------------------------------------------------------
			if (strncmp(PC_msg,"filer", 5) == 0){
				state = FILE_TRANS_1;
				
				if((files_num == 20) && (index_first != index_last)){
					index_first = (index_first + 1)%20;
					files_num -= 1;
				}
				index_last = (index_last + 1)%20;
				receive_file(index_last);   
				files_num += 1;
				
				memset(PC_msg,0,40);                                   //memset - clears the array
			}
			//---------------------------------------------------------------------------------------------------------------------
			// Files mode - AckFile
			//---------------------------------------------------------------------------------------------------------------------
			if (strncmp(PC_msg,"ackfile",7) == 0){
				int j; 
				
				//Go back to menu
				curr_index = index_last;
				next_index = print_files_menu(curr_index);
				
				DMA_DSR_BCR1 = DMA_DSR_BCR_BCR(file_size[file_to_send_idx]+1);       // number of bytes yet to be transferred
				DMA_SAR1 = (uint32_t)hd_file_Ptr[file_to_send_idx];
				UART0_C5 |= UART0_C5_TDMAE_MASK;        				  // Enable DMA request for UART0 transmitter  
				DMAMUX0_CHCFG1 |= DMAMUX_CHCFG_ENBL_MASK;				  // Enable DMA channel 
				disable_irq(INT_UART0-16);  			                  // Disable UART0 interrupt 
				
				memset(PC_msg,0,40);                                   //memset - clears the array
			}
			//---------------------------------------------------------------------------------------------------------------------
			// Files mode - print on LCD
			//---------------------------------------------------------------------------------------------------------------------
			if (strncmp(PC_msg,"FilesLCD", 8) == 0){
				curr_index = index_last;
				next_index = print_files_menu(curr_index);
				memset(PC_msg,0,40);                  //memset - clears the array
			}
			
			//---------------------------------------------------------------------------------------------------------------------
			// Exit File Transfer Mode
			//---------------------------------------------------------------------------------------------------------------------
			if (strncmp(PC_msg,"FilesExit", 9) == 0){
				
				state = IDLE_STATE_0;
				lcd_clear();
				
				memset(PC_msg,0,40);                  //memset - clears the array
			}
			
		}
	} // End of while(1)
	return 0;
}

//Modified
int print_files_menu(int file_idx){
	int j;
	char* temp_ptr;
	char str_size[5];
	int row = 0;
	int i = file_idx; //index_last;
	int print_num;
	
	lcd_clear();
	for (j=10000; j>0; j--);	 	   // Delay
	do{
		temp_ptr = file_name_Ptr[i];
		
		//Print file name using it's pointer 
		lcd_puts(temp_ptr);
		
		//Space
		lcd_cursor_right();					  
				
		//Print file's size 
		print_num = file_size[i];
		sprintf(str_size, "%d", print_num);
		lcd_puts(str_size);
		memset(str_size,0,5); 
		
		//Print 'B'
		lcd_data('B');
		
		if ( row == 0 )
		{
			lcd_new_line();        	      //second line
		}
		
		i = (i == index_first) ? index_last : ((i - 1)%20);
			
		row += 1;
	}while ((files_num > 1) && ( row < 2 ));	//Prints only 2 rows, or 1 in case it's the last file
	
	return (file_idx == index_first) ? index_last : ((file_idx - 1)%20);
}

void receive_file(int index){
		int i = 6;
		int str_size = 0;		// count size of file
		int prev_index, cont_flag;
		char* file_name_addr;
		
		lcd_clear();
		
		while (PC_msg[i] != ',')
		{
			str_size = str_size*10 + (PC_msg[i] - '0');
			i++;
		}
		file_size[index] = str_size;
		i++;
		
		if((last_addr + str_size + 30) > &files[9999])
		{
			last_addr = &files;
			while( files_num > 0 && (last_addr + str_size + 30 > hd_file_Ptr[index_first]) && index_first != index_last)
			{
				index_first = (index_first + 1)%20;
				files_num -= 1;
			}
		}
		
		
		file_name_addr = last_addr;
		file_name_Ptr[index] = file_name_addr;
		
		cont_flag = 1;
		while (cont_flag){
			*file_name_addr++ = PC_msg[i++];
			if(i > 10)
			{
				if((PC_msg[i-4] == '.') & (PC_msg[i-3] == 't') & (PC_msg[i-2] == 'x') & (PC_msg[i-1] == 't'))
					cont_flag = 0;
			}
		}
		*file_name_addr++ = '\0';
		
		//memset(PC_msg,0,40);                   //memset - clears the array
		
		hd_file_Ptr[index] = file_name_addr;
		
		*(hd_file_Ptr[index] + str_size) = '\0';
		
		last_addr = hd_file_Ptr[index] + str_size + 1;
		
		//receiving part:
		DMA_DAR0 = (uint32_t)hd_file_Ptr[index];       			//destination
		DMA_DSR_BCR0 = DMA_DSR_BCR_BCR(file_size[index]);       // number of bytes to transfer
		DMAMUX0_CHCFG0 |= DMAMUX_CHCFG_ENBL_MASK; 				// Enable DMA channel 
		disable_irq(INT_UART0-16);               			    // Disable UART0 interrupt
		UART0_C5 |= UART0_C5_RDMAE_MASK;          				// Enable DMA request for UART0 receiver
		UARTprintf(UART0_BASE_PTR,"ack\n");  
}

void print_file(int file_idx){
	int j,i,size;
	char* temp_ptr;
	
	lcd_clear();
	for (j=10000; j>0; j--);	     // Delay
	
	temp_ptr = hd_file_Ptr[file_idx];
	size = file_size[file_idx];
	i = size;
			
	//Print file name using it's pointer 
	while (i && i > (size - 32))
	{
		lcd_data(*temp_ptr++);
		if(i == (size - 15))
		{
			lcd_new_line();        	      //second line
		}
		i--;
		for (j=10000; j>0; j--);	     // Delay
	}
	
}

void send_file(){
	int j,i;
	char str[24];
	
	memset(str,0,24);                  //memset - clears the array
	sprintf(str,"filename %s\n", file_name_Ptr[file_to_send_idx]);
		
	UARTprintf(UART0_BASE_PTR,str);
	
}






