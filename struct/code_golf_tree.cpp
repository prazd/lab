#include <cstdio>
int main(){int a{};scanf("%d",&a);int b=1,c=a-1,d=c;
for (int i=1;i<a;++i){
for (int s=0;s<c;++s)printf(" ");--c;
for (int j=0;j<b;++j)printf("*");b+=2;
puts("");}
for (int i=0;i<d-1;++i)printf(" ");
puts("***");}