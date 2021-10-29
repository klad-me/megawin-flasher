#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <hidapi/hidapi.h>


//#define DUMP_PACKETS


hid_device *handle;


int qa(const uint8_t *query, uint8_t query_len, uint8_t *answer, uint8_t answer_size)
{
	if (query)
	{
#ifdef DUMP_PACKETS
		printf(">");
		for (int i=0; i<query_len; i++)
		{
			printf(" %02X", query[i]);
		}
		printf("\n");
#endif
		
		if (hid_write(handle, query, query_len) != query_len)
		{
			fprintf(stderr, "hid_write failed");
			return -1;
		}
	}
	
	if (! answer) return 0;
	
	int rx=hid_read_timeout(handle, answer, answer_size, 1000);
	if (rx < 0)
	{
		fprintf(stderr, "hid_read failed\n");
		return -1;
	}
	
#ifdef DUMP_PACKETS
	printf("<");
	for (int i=0; i<rx; i++)
	{
		printf(" %02X", answer[i]);
	}
	printf("\n");
#endif
	
	return rx;
}


uint8_t checkAdapter(void)
{
	static const uint8_t query[] ={ 0x0C, 0xA8, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	static const uint8_t answer[]={ 0x03, 0x02, 0x42, 0x01 };
	uint8_t data[8];
	
	if ( (qa(query, sizeof(query), data, sizeof(data)) != sizeof(answer)) ||
		 (memcmp(data, answer, sizeof(answer)) != 0) )
	{
		fprintf(stderr, "checkAdapter error\n");
		return 0;
	}
	
	return 1;
}


uint8_t enterICE(void)
{
	static const uint8_t query[] ={ 0x0C, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x01, 0x00 };
	static const uint8_t answer[]={ 0x01, 0x41 };
	uint8_t data[8];
	
	if ( (qa(query, sizeof(query), data, sizeof(data)) != sizeof(answer)) ||
		 (memcmp(data, answer, sizeof(answer)) != 0) )
	{
		fprintf(stderr, "enterICE error\n");
		return 0;
	}
	
	return 1;
}


uint8_t exitICE(void)
{
	static const uint8_t query[] ={ 0x0C, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	static const uint8_t answer[]={ 0x01, 0x41 };
	uint8_t data[8];
	
	if ( (qa(query, sizeof(query), data, sizeof(data)) != sizeof(answer)) ||
		 (memcmp(data, answer, sizeof(answer)) != 0) )
	{
		fprintf(stderr, "exitICE error\n");
		return 0;
	}
	
	return 1;
}


// Вероятно - определение типа процессора
uint8_t unknown1(void)
{
	static const uint8_t query[] ={ 0x0C, 0x93, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	static const uint8_t answer[]={ 0x03, 0x51, 0xD0, 0x05 };
	uint8_t data[8];
	
	if ( (qa(query, sizeof(query), data, sizeof(data)) != sizeof(answer)) ||
		 (memcmp(data, answer, sizeof(answer)) != 0) )
	{
		fprintf(stderr, "unknown1 error\n");
		return 0;
	}
	
	return 1;
}


uint8_t eraseFlash(void)
{
	static const uint8_t query[] ={ 0x0C, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	static const uint8_t answer[]={ 0x01, 0x41 };
	uint8_t data[8];
	
	if ( (qa(query, sizeof(query), data, sizeof(data)) != sizeof(answer)) ||
		 (memcmp(data, answer, sizeof(answer)) != 0) )
	{
		fprintf(stderr, "eraseFlash error\n");
		return 0;
	}
	
	return 1;
}


uint8_t writeFuse(const uint8_t *fuse)
{
	uint8_t query[22] ={ 0x15, 0x9C, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	static const uint8_t answer[]={ 0x01, 0x41 };
	uint8_t data[8];
	
	memcpy(query+12, fuse, 9);
	query[sizeof(query)-1]=0;
	
	if ( (qa(query, sizeof(query), data, sizeof(data)) != sizeof(answer)) ||
		 (memcmp(data, answer, sizeof(answer)) != 0) )
	{
		fprintf(stderr, "writeFuse error\n");
		return 0;
	}
	
	return 1;
}


uint8_t readFlash(uint32_t addr, uint8_t *data, uint32_t size)
{
	uint8_t query[13]={ 0x0C, 0x82, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	uint8_t pkt[64];
	memcpy(query+2, &addr, 4);
	memcpy(query+6, &size, 4);
	
	if (qa(query, sizeof(query), 0, 0) != 0)
	{
		fprintf(stderr, "readFlash query error\n");
		return 0;
	}
	
	while (size > 0)
	{
		int len=qa(0, 0, pkt, sizeof(pkt));
		if (len <= 0)
		{
			fprintf(stderr, "readFlash answer error\n");
			return 0;
		}
		if (len != pkt[0]+1)
		{
			fprintf(stderr, "readFlash answer data error\n");
			return 0;
		}
		len--;
		
		if (len > size) len=size;
		memcpy(data, pkt+1, len);
		data+=len;
		size-=len;
	}
	
	return 1;
}


uint8_t writeFlash(uint32_t addr, const uint8_t *data, uint32_t size)
{
	uint8_t query[0x3D]={ 0x3C, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	static const uint8_t answer[]={ 0x01, 0x41 };
	uint8_t pkt[64];
	query[sizeof(query)-1]=0;
	
	while (size > 0)
	{
		uint32_t data_len=(size > 0x30) ? 0x30 : size;
		
		memcpy(query+2, &addr, 4);
		memcpy(query+6, &data_len, 4);
		memcpy(query+12, data, data_len);
		
		if ( (qa(query, 12+data_len+1, pkt, sizeof(pkt)) != sizeof(answer)) ||
			 (memcmp(pkt, answer, sizeof(answer)) != 0) )
		{
			fprintf(stderr, "writeFlash answer error\n");
			return 0;
		}
		
		addr+=data_len;
		data+=data_len;
		size-=data_len;
	}
	
	return 1;
}


void usage(const char *argv0)
{
	fprintf(stderr, "Usage: %s [options]\n", argv0);
	fprintf(stderr, "  -e / --erase              Erase flash\n");
	fprintf(stderr, "  -w / --write  <file.bin>  Write flash (without erase !)\n");
	fprintf(stderr, "  -r / --read   <file.bin>  Read flash\n");
	fprintf(stderr, "  -v / --verify             Verify after write\n");
	fprintf(stderr, "  -a / --addr   <address>   Set base address (default=0)\n");
	fprintf(stderr, "  -s / --size   <size>      Set size (default=file size|entire flash)\n");
	fprintf(stderr, "  -f / --fuse   <fuse-list> Comma-separated fuse list\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Supported fuses:\n");
	fprintf(stderr, "  HWBS   Run ISP on power on\n");
	fprintf(stderr, "  HWBS2  Run ISP on power on & external reset\n");
	fprintf(stderr, "  IAP=X  Set IAP size to 0k to 16k (in 0.5k steps)\n");
	fprintf(stderr, "  ISP=X  Set ISP size to 0k to 7.5k (in 0.5k steps)\n");
	fprintf(stderr, "  BOD1=X Set BOD1 voltage (2.0, 2.4, 3.7, 4.2)\n");
	fprintf(stderr, "  BO0REO Trigger reset on BOD0\n");
	fprintf(stderr, "  BO1REO Trigger reset on BOD1\n");
	fprintf(stderr, "  WRENO  Set WDTCR.WREN (enable WDT reset)\n");
	fprintf(stderr, "  NSWDT  Set WDTCR.NSW (enable WDT in power down mode)\n");
	fprintf(stderr, "  HWENW  Enable WDT\n");
	fprintf(stderr, "  WDSFWP Write-protect WDT registers\n");
#warning TODO: wdt values (datasheet p.287)
}


int main(int argc, char **argv)
{
	int ret=-1;
	FILE *Fwrite=0, *Fread=0;
	uint8_t erase=0, verify=0;
	uint16_t addr=0, size=16384;
	uint8_t fuse[]={ 0x08, 0xe3, 0xff, 0x00, 0x68, 0xe1, 0xff, 0xff, 0x05};
	int iap_size=0, isp_size=0;
	
	if (argc < 2)
	{
		usage(argv[0]);
		return -1;
	}
	
	struct option opts[]=
	{
		{ "erase",		no_argument,			0,		'e' },
		{ "write",		required_argument,		0,		'w' },
		{ "read",		required_argument,		0,		'r' },
		{ "verify",		no_argument,			0,		'v' },
		{ "addr",		required_argument,		0,		'a' },
		{ "size",		required_argument,		0,		's' },
		{ "fuse",		required_argument,		0,		'f' },
		{ 0 }
	};
	int opt;
	while ( (opt=getopt_long(argc, argv, "e-w:r:v-a:s:f:", opts, 0)) > 0)
	{
		switch (opt)
		{
			case 'e':
				erase=1;
				break;
			
			case 'w':
				Fwrite=fopen(optarg, "rb");
				if (! Fwrite)
				{
					perror(optarg);
					return -1;
				}
				break;
			
			case 'r':
				Fread=fopen(optarg, "wb");
				if (! Fread)
				{
					perror(optarg);
					return -1;
				}
				break;
			
			case 'v':
				verify=1;
				break;
			
			case 'a':
				if (! strcasecmp(optarg, "0x"))
					addr=strtol(optarg, 0, 16); else
					addr=atoi(optarg);
				break;
			
			case 's':
				if (! strcasecmp(optarg, "0x"))
					size=strtol(optarg, 0, 16); else
					size=atoi(optarg);
				break;
			
			case 'f':
				{
					char *ss=optarg;
					uint8_t ok=1;
					while (*ss)
					{
						// Пропускаем пробелы
						while (isspace(*ss)) ss++;
						if (! *ss) break;
						
						// Получаем название и значение
						if (! isalnum(*ss))
						{
							ok=0;
							break;
						}
						const char *name=ss, *value;
						while (isalnum(*ss)) ss++;
						if ((*ss) == '=')
						{
							// Есть значение
							(*ss++)=0;
							value=ss;
							while ( (*ss) && ((*ss) != ',') && (! isspace(*ss)) ) ss++;
							if (*ss) (*ss++)=0;
						} else
						{
							// Нет значения
							if (*ss) (*ss++)=0;
							value="";
						}
						
						// Обрабатываем опцию
						if (! strcasecmp(name, "HWBS")) fuse[0]&=~0x08; else
						if (! strcasecmp(name, "HWBS2")) fuse[1]&=~0x20; else
						if (! strcasecmp(name, "BOD1"))
						{
							uint8_t bits;
							switch ( (int)(atof(value)*10) )
							{
								case 20:	bits=0; break;
								case 24:	bits=1; break;
								case 37:	bits=2; break;
								case 42:	bits=3; break;
								default:
									fprintf(stderr, "Incorrect BOD1 value\n");
									return -1;
							}
							fuse[1]=(fuse[1] & ~0x03) | bits;
						} else
						if (! strcasecmp(name, "BO0REO")) fuse[1]&=~0x40; else
						if (! strcasecmp(name, "BO1REO")) fuse[1]&=~0x80; else
						if (! strcasecmp(name, "WRENO")) fuse[2]&=~0x10; else
						if (! strcasecmp(name, "HWENW")) fuse[2]&=~0x20; else
						if (! strcasecmp(name, "NSWDT")) fuse[2]&=~0x40; else
						if (! strcasecmp(name, "WDSFWP")) fuse[2]&=~0x80; else
						if (! strcasecmp(name, "IAP")) iap_size=(int)(atof(value)*10); else
						if (! strcasecmp(name, "ISP")) isp_size=(int)(atof(value)*10); else
						{
							fprintf(stderr, "Incorrect fuse '%s'\n", name);
							return -1;
						}
					}
					
					if (! ok)
					{
						fprintf(stderr, "Incorrect fuse list !\n");
						return -1;
					}
					
					if ( (isp_size < 0) || (isp_size > 75) || ((isp_size%5) != 0) )
					{
						fprintf(stderr, "Incorrect ISP size\n");
						return -1;
					}
					
					if ( (iap_size < 0) || (iap_size > 160) || ((iap_size%5) != 0) || (isp_size+iap_size > 160) )
					{
						fprintf(stderr, "Incorrect IAP size\n");
						return -1;
					}
					
					const uint8_t isp_tab[]={0xf6, 0xd6, 0xb6, 0x96, 0x76, 0x56, 0x36, 0x16, 0xf4, 0xd4, 0xb4, 0x94, 0x74, 0x54, 0x34, 0x14};	// 0-7.5k
					const uint8_t iap_tab[]={0x41, 0x3f, 0x3d, 0x3b, 0x39, 0x37, 0x35, 0x33, 0x31, 0x2f, 0x2d, 0x2b, 0x29, 0x27, 0x25, 0x23,
											 0x21, 0x1f, 0x1d, 0x1b, 0x19, 0x17, 0x15, 0x13, 0x11, 0x0f, 0x0d, 0x0b, 0x09, 0x07, 0x05, 0x03, 0x01};	// 0-16k
					
					// Получаем индексы к таблице
					isp_size/=5;
					iap_size/=5;
					fuse[0]|=isp_tab[isp_size];
					fuse[3]|=iap_tab[isp_size+iap_size];
				}
				break;
			
			case '?':
			default:
				usage(argv[0]);
				return -1;
		}
	}
	
	if ((argc-optind) > 0)
	{
		fprintf(stderr, "%s: too many artuments\n", argv[0]);
		return -1;
	}
	
	
	if (Fwrite && Fread)
	{
		fprintf(stderr, "Read and write at the same time !\n");
		return -1;
	}
	
	if (addr >= 16384)
	{
		fprintf(stderr, "Bad address\n");
		return -1;
	}
	
	if ( (size == 0) || (addr+size > 16384) )
	{
		fprintf(stderr, "Bad size\n");
		return -1;
	}
	
	
	// Открываем устройство
	if (hid_init() != 0)
	{
		fprintf(stderr, "hid_init failed\n");
		return -1;
	}
	
	handle = hid_open(0x0e6a, 0x030a, NULL);
	if (! handle)
	{
		fprintf(stderr, "Megawin ICE not found !\n");
		return -1;
	}
	
	// Переходим в режим программирования
	printf("Entering programming mode...\n");
	if (! checkAdapter()) goto done;
	if (! enterICE()) goto done;
	
	// Возможно, проверка типа чипа
	if (! unknown1()) goto done;
	
	// Стираем флэш
	if (erase)
	{
		printf("Erasing flash...\n");
		if (! eraseFlash()) goto done;
	}
	
	// Записываем fuse
	printf("Writing FUSE...\n");
	if (! writeFuse(fuse)) goto done;
	
	// Записываем флэш
	if (Fwrite)
	{
		uint8_t data[16384];
		int len=fread(data, 1, sizeof(data), Fwrite);
		fclose(Fwrite);
		if (size > len) size=len;
		
		// Записываем
		printf("Writing flash...\n");
		if (! writeFlash(addr, data, size)) goto done;
		
		// Проверяем, если надо
		if (verify)
		{
			uint8_t data2[16384];
			printf("Verifying flash...\n");
			if (! readFlash(addr, data2, size)) goto done;
			if (memcmp(data, data2, size) != 0)
			{
				fprintf(stderr, "Verification failed !\n");
				goto done;
			}
		}
	}
	
	// Читаем флэш
	if (Fread)
	{
		uint8_t data[16384];
		printf("Reading flash...\n");
		if (! readFlash(addr, data, size)) goto done;
		fwrite(data, 1, size, Fread);
		fclose(Fread);
	}
	
	
	// Запускаем программу
	printf("Running program...\n");
	if (! exitICE()) goto done;
	
	// Все хорошо
	printf("Done\n");
	ret=0;
	
	
done:
	hid_close(handle);
	
	return ret;
}
