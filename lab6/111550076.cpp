/*
Student No.: 111550076
Student Name: 楊子賝
Email: zichen55.cs11@nycu.edu.tw
SE tag: xnxcxtxuxoxsx
Statement: I am fully aware that this program is not
supposed to be posted to a public server, such as a
public GitHub repository or a public web page.
*/

#define FUSE_USE_VERSION 30
#include <fuse.h>
#include <string.h>
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string>
using namespace std;

vector<string> splitPath(const string& path) {
    vector<string> parts;
    regex re("/+");  
    sregex_token_iterator it(path.begin(), path.end(), re, -1);
    sregex_token_iterator end;
    for (; it != end; ++it) {
        if (!it->str().empty()) {
            parts.push_back(it->str());
        }
    }
    return parts;
}
struct header_posix_ustar {
        char name[100];
        char mode[8];
        char uid[8];
        char gid[8];
        char size[12];
        char mtime[12];
        char checksum[8];
        char typeflag[1];
        char linkname[100];
        char magic[6];
        char version[2];
        char uname[32];
        char gname[32];
        char devmajor[8];
        char devminor[8];
        char prefix[155];
        char pad[12];
};

struct tar_entry {  
    struct header_posix_ustar header;
    string content;  
    map<string, tar_entry*> children;
};

tar_entry* root = new tar_entry();
struct tar_entry* find(const string& path) {
    vector<string> paths = splitPath(path);
    tar_entry* current = root;
    for (const auto& p : paths) {
        auto it = current->children.find(p);
        if (it == current->children.end()) {
            return nullptr;  
        }
        current = it->second;  
    }
    return current;
}


int my_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    filler(buffer, ".", NULL, 0); 
    filler(buffer, "..", NULL, 0);
    tar_entry* dir = find(path);
    if(!dir){
        return -ENOENT;
    }
    for_each(dir->children.begin(), dir->children.end(), [&filler, &buffer](const pair<string, tar_entry*>& entry) {
        filler(buffer, entry.first.c_str(), NULL, 0);
    });
    return 0;
}

int my_getattr(const char *path, struct stat *st) {
    string path_(path);
    if (path_ == "/") {
        st->st_mode = S_IFDIR | 0444;
        // printf("root_mode=%o\n\n", st->st_mode);
        return 0;
    }

    tar_entry *entry = find(path);
    if (entry == NULL) {
        return -ENOENT;
    }
    st->st_mode = strtol(entry->header.mode, NULL, 8);
    st->st_uid = strtol(entry->header.uid, NULL, 8);
    st->st_gid = strtol(entry->header.gid, NULL, 8);
    st->st_size = strtol(entry->header.size, NULL, 8);
    st->st_mtime = strtol(entry->header.mtime, NULL, 8);
    st->st_nlink = 0;
    st->st_blocks = 0;
    char typeflag = entry->header.typeflag[0];
    if(typeflag == '5'){
        st->st_mode |= S_IFDIR;
    }
    else if (typeflag == '2') {
        st->st_mode |= S_IFLNK;
    } else {
        st->st_mode |= S_IFREG;
    }
    return 0;  
}


int my_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
    tar_entry *entry = find(path);
    if(entry == NULL){
        return -EINVAL;
    }
    char typeflag = entry->header.typeflag[0];
    if(typeflag == '5'){
        return -EISDIR;
    }
    // printf("my_read: path = %s, dir = %s, file = %s\n", path, dir.c_str(), file_.c_str());
    string content = entry->content;
    size_t content_size = strtol(entry->header.size, NULL, 8);
    if(offset >= content_size){
        return 0;
    }
    if(offset + size > content_size){
        memcpy(buffer, content.c_str() + offset, content_size - offset);
        return content_size - offset;
    }
    memcpy(buffer, content.c_str() + offset, size);
    return size;
}


int my_readlink(const char *path, char *buffer, size_t size) {
    tar_entry *entry = find(path);

    if(entry == NULL){
        return -EINVAL;
    }
    string linkname = entry->header.linkname;
    if (linkname.size() >= size) {
        return -ENAMETOOLONG;
    }
    memcpy(buffer, linkname.c_str(), linkname.size() + 1);
    return 0;
}


static struct fuse_operations op ;


int main(int argc, char *argv[]){
    FILE *file = fopen("test.tar", "rb");
    if (!file) {
        perror("Failed to open tar file");
        return -1;
    }
 
    char end_str[512] = {0};
    while (true) {
        struct header_posix_ustar tar_header;
        size_t read_size = fread(&tar_header, 1, 512, file);
        
        if (memcmp(&tar_header, end_str, 512) == 0) {
            size_t next_read  = fread(&tar_header, 1, 512, file);
            if (next_read  == 0 || memcmp(&tar_header, end_str, 512) == 0) {
                break;
            }
        }

        unsigned long file_size = strtoul(tar_header.size, NULL, 8);
        
        char *content = new char[file_size + 1];
        size_t content_read = fread(content, 1, file_size, file);
        if (content_read != file_size) {
            delete[] content;
            fclose(file);
            return -EIO;
        }
        content[file_size] = '\0';

        string filename(tar_header.name);
        filename.insert(0, "/");
        vector<string> paths = splitPath(filename);
        tar_entry* current = root;
        for (const auto& p : paths) {
            if (current->children.count(p) == 0) {
                current->children[p] = new tar_entry();
            }
            current = current->children[p];
        }
        current->header = tar_header;
        current->content = std::string(content, file_size);
        delete[] content;  

        unsigned long skip = (512 - (file_size % 512)) % 512;
        if (skip > 0) {
            if (fseek(file, skip, SEEK_CUR) != 0) {
                fclose(file);
                return -EIO;
            }
        }
    }

    fclose(file);
    memset(&op, 0, sizeof(op)); 
    op.getattr = my_getattr;
    op.readdir = my_readdir;
    op.read = my_read;
    op.readlink = my_readlink;
    return fuse_main(argc, argv, &op, NULL);
}
