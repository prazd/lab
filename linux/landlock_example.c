#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <linux/landlock.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <assert.h>

void __attribute__((constructor)) init(){
    struct landlock_ruleset_attr ruleset = {
            .handled_access_fs = LANDLOCK_ACCESS_FS_READ_FILE | 
                                 LANDLOCK_ACCESS_FS_WRITE_FILE| 
                                 LANDLOCK_ACCESS_FS_READ_DIR  |
                                 LANDLOCK_ACCESS_FS_REMOVE_FILE|
                                 LANDLOCK_ACCESS_FS_MAKE_REG,
    };
    int ruleset_fd = syscall(__NR_landlock_create_ruleset, &ruleset, sizeof(ruleset), 0);
    if (ruleset_fd < 0) abort();

    int fd = open("f.txt", O_CREAT | O_RDWR, 0666);
    if (fd < 0) abort();
    write(fd, "abc", 3);
    struct landlock_path_beneath_attr f_attr = {
        .allowed_access = LANDLOCK_ACCESS_FS_READ_FILE,
        .parent_fd = fd,
    };
    syscall(__NR_landlock_add_rule, ruleset_fd,LANDLOCK_RULE_PATH_BENEATH, &f_attr, 0);
    close(fd);

    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0)){
        close(ruleset_fd);
        abort();
    }

    if (syscall(__NR_landlock_restrict_self,ruleset_fd,0)){
        close(ruleset_fd);
        abort();
    }
    close(ruleset_fd);
}


int main(){
    int fd = open("f.txt", O_RDONLY);
    if (fd < 0) return -1;
    char res[4];
    read(fd, res, 3);
    printf("%s\n", res);
    close(fd);

    assert(open("f.txt", O_RDWR) < 0);
}
