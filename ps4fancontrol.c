#include <sys/ioctl.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/sysmacros.h>


uint8_t curTemp = 0x4f;
uint8_t prevTemp = 0;

char *configFile;

int debug = -1;

#define ICC_MAJOR	'I'

struct icc_cmd {
	
	uint8_t major;
	uint16_t minor;
	uint8_t *data;
	uint16_t data_length;
	uint8_t *reply;
	uint16_t reply_length;
};

#define ICC_IOCTL_CMD _IOWR(ICC_MAJOR, 1, struct icc_cmd)

int getUserGroupId(int *uid, int *gid)
{
	size_t len = 0;
	ssize_t read;
	char *buf = NULL;
	
	*uid = -1;
	*gid = -1;

    FILE *f = fopen("/etc/passwd", "r");
    if(f == NULL)
    {
		perror("Error");
		exit(-1);
	}
	
	while((read = getline(&buf, &len, f)) != -1) 
	{
		int foundHome = -1;
		int foundBin = -1;
		
		for(int i = 0; i < read; i++)
		{	
			char *line = malloc(read);
			line = buf;
			
			if(strncmp((line + i), ":/home", 6) == 0)
			{
				foundHome = 0;
			}
			if(strncmp((line + i), ":/bin/", 6) == 0)
			{
				foundBin = 0;
			}
			
			if(foundBin == 0 && foundHome == 0)
			{	
				for(int n = 0; n < i; n++)
				{
					if(*uid == -1)
					{
						if(strncmp((line + n), ":x:", 3) == 0)
						{
							*uid = atoi((line + n + 3));
							if(!debug)
								printf("uid: %d\n", *uid);
							n += 3;
						}
					}
					else
					{
						if(*(line + n) == ':')
						{
							*gid = atoi((line + n + 1));
							if(!debug)
								printf("gid: %d\n", *gid);
							fclose(f);
							return 0;
						}
					}
				}
			}
		}
    }
    
    return -1;
}

int file_exist(const char *filename)
{	
	FILE *f = fopen(filename, "rb");
	if(f == NULL)
		return -1;
	
	fclose(f);
	return 0;
}

int initSettings()
{
	char *configDir;
	char *tmp_buffer;
	const char *homeDir;
	struct passwd *pwd;
	
	pwd = getpwuid(getuid());
	homeDir = pwd->pw_dir;
		
	if(!debug)
		printf("Home directory is %s\n", homeDir);
		
	tmp_buffer = malloc(strlen(homeDir) + 10);
	sprintf(tmp_buffer, "%s/.config", homeDir);
	
	if(!debug)
		printf("Config directory is %s\n", tmp_buffer);
	
	DIR *dir = opendir(tmp_buffer);
	if(dir == NULL)
	{
		printf("Directory %s not found\n",tmp_buffer );
		if(errno == ENOENT)
		{
			printf("Create directory %s\n", tmp_buffer);
			if(mkdir(tmp_buffer, 0700))
			{
				perror("Error");
				return -1;
			}
		}
	}
	closedir(dir);
	
	configDir = malloc(strlen(tmp_buffer) + 16);
	sprintf(configDir, "%s/Ps4FanControl", tmp_buffer);
	
	dir = opendir(configDir);
	if(dir == NULL)
	{
		printf("Directory %s not found\n", configDir);
		if(errno == ENOENT)
		{
			printf("Create directory %s\n", configDir);
			if(mkdir(configDir, 0755))
			{
				perror("Error");
				return -1;
			}
		}
	}
	closedir(dir);
	
	configFile = malloc(strlen(configDir) + 16);
	sprintf(configFile, "%s/threshold_temp", configDir);
	free(configDir);
	
	return 0;
}

int saveConfig(uint8_t temperature)
{
	FILE *f = fopen(configFile, "wb");
	if(f == NULL)
	{
		perror("Error");
		return -1;
	}
	if(fwrite(&temperature, 1, 1, f) != 1)
	{
		printf("Error writing configuration file\n");
		fclose(f);
		unlink(configFile);
		return -1;
	}
	
	if(!debug)
		printf("Selected threshold temperature saved in %s\n", configFile);
	
	fclose(f);
	
	return 0;
}

int loadConfig()
{
	FILE *f;
	uint8_t ret;
	uint8_t temp_bak = curTemp;
	
	f = fopen(configFile, "rb");
	if(f == NULL)
	{
		printf("Configuration file not found\n");
		return -1;
	}
	
	fseek(f, 0, SEEK_CUR);
	
	if(fread(&ret, 1, 1, f) != 1)
	{
		printf("Error reading configuration file\n");
		curTemp = temp_bak;
		fclose(f);
		return -1;
	}
	
	if(!debug)
		printf("ret value: %d\n", ret);
	
	if(ret >= 45 && ret <= 85)
		curTemp = ret;
	else
		printf("Configuration file contain a invalid value\n");
	
	if(!debug)
		printf("Threshold temperature loaded from configuration file\n");
	
	fclose(f);
	
	return 0;
}


int set_temp_threshold(uint8_t temperature)
{
	int fd = -1;
	int ret = 0;
	struct icc_cmd arg;
	uint8_t data[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x4f};

	fd = open("/dev/icc", 0);
	if(fd == -1)
	{
		perror("Error");
		return -1;
	}
	
	arg.major = 0xA;
	arg.minor = 0x6;
	arg.data = malloc(sizeof(data));
	arg.data_length = 0x34;
	arg.reply = malloc(0x20);
	arg.reply_length = 0x20;
	
	//one more useless check
	if(temperature >= 45 && temperature <= 85)
		data[5] = temperature;
	else
	{
		printf("Current threshold temperature (%u°C) is out of range, back to default (79°C)\n", temperature);
		temperature = 0x4f;
	}
	
	memcpy(arg.data, data, sizeof(data));
	
	ret = ioctl(fd, ICC_IOCTL_CMD, &arg);
	usleep(1000);
	
	if (!debug)
		printf("ioctl ret: %d", ret);
	
	switch(ret)
	{
		case -EFAULT:
			printf(" Error: Bad address");
			return ret;
		case -ENOENT:
			printf(" Error: Invalid command");
			return ret;
		case -1:
			perror(" Error");
			return ret;
		default:
			break;
	}
	
	if(!debug)
	{
		printf("\nReply: ");
		for(int i = 0; i < ret; i++)
		{
			printf("0x%02x ", *(arg.reply + i));
		}
	}
	
	switch(*arg.reply)
	{
		case 0x00:
			if(!debug)
				printf("\nSuccess!\n");
			break;
		case 0x02:
			printf("\nError: Invalid data\n");
			return *arg.reply;
		case 0x04:
			printf("\nError: Invalid data length\n");
			return *arg.reply;
		default:
			printf("\nUnknown status code: 0x%02x\n", *arg.reply);
			return *arg.reply;
	}
	return 0;
}

int get_temp_threshold(uint8_t* temp)
{
	int fd = -1;
	int ret = 0;
	struct icc_cmd arg;
	
	*temp = 0x4f;

	fd = open("/dev/icc", 0);
	if(fd == -1)
	{
		perror("Error");
		return -1;
	}
	
	arg.major = 0xA;
	arg.minor = 0x7;
	arg.data = 0;
	arg.data_length = 0;
	arg.reply = malloc(0x52);
	arg.reply_length = 0x52;
	
	ret = ioctl(fd, ICC_IOCTL_CMD, &arg);
	usleep(1000);
	
	if(!debug)
		printf("ioctl ret: %d", ret);
	
	switch(ret)
	{
		case -EFAULT:
			printf(" Error: Bad address");
			return ret;
		case -ENOENT:
			printf(" Error: Invalid command");
			return ret;
		case -1:
			perror(" Error");
			return ret;
		default:
			break;
	}
	
	if(!debug)
	{
		printf("\nReply: ");
		for(int i = 0; i < ret; i++)
		{
			printf("0x%02x ", *(arg.reply + i));
		}
	}
	
	switch(*arg.reply)
	{
		case 0x00:
			if(!debug)
				printf("\nSuccess!\n");
			break;
		case 0x02:
			printf("\nError: Invalid data\n");
			return *arg.reply;
		case 0x04:
			printf("\nError: Invalid data length\n");
			return *arg.reply;
		default:
			printf("\nUnknown status code: 0x%02x\n", *arg.reply);
			return *arg.reply;
	}
	
	*temp = *(arg.reply + 5);
		
	return 0;
}


int main(int argc, char *argv[])
{
	int nextTemp = 70;
	int ret = -1;
	int res = -1;
	int gid;
	int uid;
	dev_t dev;

	if(argc < 2)
	{
		printf("You have to provide temperature argument! (See --help) \n");
		return -1;
	}
	
	for(int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
		{
			printf("Usage: ps4fancontrol temp ");
			printf("[-h | --help] [-d | --debug] [-r | --reset]\n"); 
			return 0;
		}
		else if(strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0)
		{
			debug = 0;
		}
		else if(strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--reset") == 0)
		{
			res = 0;
		}
		else 
		{
			if(argv[i][0] == '-')
			{
				printf("Invalid argument at %d given: %s\n", i, argv[i]);
				return -1;
			}
			nextTemp = atoi(argv[i]);
		}
	}
		
	dev = makedev(0x49, 1);
	mknod("/dev/icc", S_IFCHR | 0666, dev);
	
	int fd = open("/dev/icc", 0);
	if(fd == -1)
	{
		printf("Error: you need run the program as root!");		
		return -1;
	}
	close(fd);
	
	//drop root priviliges
	if(getUserGroupId(&uid, &gid))
	{
		printf("Uid and gid not found, can't drop privileges\n");
		return -1;
	}
	
	setgid(gid);
	setuid(uid);
	
	if(setuid(0) == 0)
	{
		printf("Error: we still root, this is bad\n");
		return -1;
	}
	
	initSettings();
	ret = loadConfig();

	get_temp_threshold(&prevTemp);
	curTemp = nextTemp;

	if (prevTemp == curTemp)
	{
		printf("Threshold temperature already at: %d\n", curTemp);
		return 0;
	}
	
	printf("Threshold temperature set %d --> %d\n", prevTemp, curTemp);

	if (ret != -1 && !file_exist(configFile)) 
	{
		saveConfig(curTemp);
	}

	if(set_temp_threshold(curTemp))
		return -1;
	else
		return 0;

	return 0;
}
