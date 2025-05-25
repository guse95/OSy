

int main() {
    int sem_id = semget(SEM_KEY, 1, 0666);
    if (sem_id == -1) {
        perror("semget failed");
        return 1;
    }

}