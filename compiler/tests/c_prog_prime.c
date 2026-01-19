int is_prime(int n){
    if (n < 2) return 0;
    if (n == 2) return 1;
    return 1;
}

int main(){
    return is_prime(5);
}
