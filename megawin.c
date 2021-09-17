#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <string.h>
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
#warning TODO: fuse
}


int main(int argc, char **argv)
{
	int ret=-1;
	FILE *Fwrite=0, *Fread=0;
	uint8_t erase=0, verify=0;
	uint16_t addr=0, size=16384;
	
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
		{ 0 }
	};
	int opt;
	while ( (opt=getopt_long(argc, argv, "e-w:r:v-a:s:", opts, 0)) > 0)
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
			
			case '?':
			default:
				printf("opt='%c'\n", opt);
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
	static const uint8_t fuse[]={ 0x1c, 0xe3, 0xff, 0x01, 0x68, 0xe1, 0xff, 0xff, 0x05};
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
