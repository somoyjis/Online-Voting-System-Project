/*
Features:
 - Admin (master) login (default: admin / admin123) -> change inside file
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define STUDENT_FILE "students.txt"
#define CANDIDATE_FILE "candidates.txt"
#define VOTES_FILE "votes.txt"
#define RESULTS_FILE "results.csv"

#define MAX_LINE 512
#define MAX_ID 64
#define MAX_NAME 100
#define MAX_PASS 64


typedef struct Student {
    char student_id[MAX_ID];
    char name[MAX_NAME];
    char pass_hash[MAX_PASS];
    int hasVoted;
    struct Student *next;
} Student;

typedef struct Candidate {
    int candidate_id;
    char name[MAX_NAME];
    char department[64];
    int votes;
    struct Candidate *next;
} Candidate;

typedef struct VoteRecord {
    char student_id[MAX_ID];
    int candidate_id;
    char timestamp[64];
    struct VoteRecord *next;
} VoteRecord;


Student *students_head = NULL;
Candidate *candidates_head = NULL;
VoteRecord *votes_head = NULL;

int election_active = 1;
int next_candidate_id = 1;


unsigned long simple_hash(const char *str) {

    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) hash = ((hash << 5) + hash) + c;
    return hash;
}

void hash_to_hex(unsigned long h, char *out, size_t out_len) {
    snprintf(out, out_len, "%08lX", h);
}

char *trim_newline(char *s) {
    if (!s) return s;
    size_t l = strlen(s);
    if (l == 0) return s;
    if (s[l-1] == '\n' || s[l-1] == '\r') s[l-1] = '\0';
    if (l>1 && (s[l-2]=='\r')) s[l-2]='\0';
    return s;
}


void load_students() {
    FILE *f = fopen(STUDENT_FILE, "r");
    if (!f) return;
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        trim_newline(line);
        if (strlen(line) == 0) continue;

        char *sid = strtok(line, ",");
        char *name = strtok(NULL, ",");
        char *ph = strtok(NULL, ",");
        char *hv = strtok(NULL, ",");
        if (!sid || !name || !ph || !hv) continue;
        Student *s = malloc(sizeof(Student));
        strncpy(s->student_id, sid, MAX_ID);
        strncpy(s->name, name, MAX_NAME);
        strncpy(s->pass_hash, ph, MAX_PASS);
        s->hasVoted = atoi(hv);
        s->next = students_head;
        students_head = s;
    }
    fclose(f);
}

void save_students() {
    FILE *f = fopen(STUDENT_FILE, "w");
    if (!f) { perror("Error saving students"); return; }
    Student *p = students_head;
    while (p) {
        fprintf(f, "%s,%s,%s,%d\n", p->student_id, p->name, p->pass_hash, p->hasVoted);
        p = p->next;
    }
    fclose(f);
}

void load_candidates() {
    FILE *f = fopen(CANDIDATE_FILE, "r");
    if (!f) return;
    char line[MAX_LINE];
    int max_id = 0;
    while (fgets(line, sizeof(line), f)) {
        trim_newline(line);
        if (strlen(line) == 0) continue;

        char *id = strtok(line, ",");
        char *name = strtok(NULL, ",");
        char *dept = strtok(NULL, ",");
        char *v = strtok(NULL, ",");
        if (!id || !name || !dept || !v) continue;
        Candidate *c = malloc(sizeof(Candidate));
        c->candidate_id = atoi(id);
        if (c->candidate_id > max_id) max_id = c->candidate_id;
        strncpy(c->name, name, MAX_NAME);
        strncpy(c->department, dept, sizeof(c->department));
        c->votes = atoi(v);
        c->next = candidates_head;
        candidates_head = c;
    }
    next_candidate_id = max_id + 1;
    fclose(f);
}

void save_candidates() {
    FILE *f = fopen(CANDIDATE_FILE, "w");
    if (!f) { perror("Error saving candidates"); return; }
    Candidate *p = candidates_head;
    while (p) {
        fprintf(f, "%d,%s,%s,%d\n", p->candidate_id, p->name, p->department, p->votes);
        p = p->next;
    }
    fclose(f);
}

void load_votes() {
    FILE *f = fopen(VOTES_FILE, "r");
    if (!f) return;
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        trim_newline(line);
        if (strlen(line) == 0) continue;

        char *sid = strtok(line, ",");
        char *cid = strtok(NULL, ",");
        char *ts = strtok(NULL, ",");
        if (!sid || !cid || !ts) continue;
        VoteRecord *r = malloc(sizeof(VoteRecord));
        strncpy(r->student_id, sid, MAX_ID);
        r->candidate_id = atoi(cid);
        strncpy(r->timestamp, ts, sizeof(r->timestamp));
        r->next = votes_head;
        votes_head = r;
    }
    fclose(f);
}

void save_vote_record(VoteRecord *rec) {
    FILE *f = fopen(VOTES_FILE, "a");
    if (!f) { perror("Error saving vote"); return; }
    fprintf(f, "%s,%d,%s\n", rec->student_id, rec->candidate_id, rec->timestamp);
    fclose(f);
}


Student *find_student_by_id(const char *id) {
    Student *p = students_head;
    while (p) {
        if (strcmp(p->student_id, id) == 0) return p;
        p = p->next;
    }
    return NULL;
}

Candidate *find_candidate_by_id(int id) {
    Candidate *p = candidates_head;
    while (p) {
        if (p->candidate_id == id) return p;
        p = p->next;
    }
    return NULL;
}

int is_student_registered(const char *id) {
    return find_student_by_id(id) != NULL;
}

int verify_password(Student *s, const char *password) {
    char hex[32];
    unsigned long h = simple_hash(password);
    hash_to_hex(h, hex, sizeof(hex));
    return strcmp(hex, s->pass_hash) == 0;
}

void current_time_str(char *buf, size_t n) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(buf, n, "%Y-%m-%d %H:%M:%S", tm);
}


void register_student() {
    char sid[MAX_ID], name[MAX_NAME], pass[MAX_PASS];
    printf("\n=== Student Registration ===\n");
    printf("Enter Student ID: "); fgets(sid, sizeof(sid), stdin); trim_newline(sid);
    if (strlen(sid) == 0) { printf("ID cannot be empty.\n"); return; }
    if (is_student_registered(sid)) { printf("Student ID already registered.\n"); return; }
    printf("Enter Name: "); fgets(name, sizeof(name), stdin); trim_newline(name);
    printf("Set Password: "); fgets(pass, sizeof(pass), stdin); trim_newline(pass);


    Student *s = malloc(sizeof(Student));
    strncpy(s->student_id, sid, MAX_ID);
    strncpy(s->name, name, MAX_NAME);
    unsigned long h = simple_hash(pass);
    hash_to_hex(h, s->pass_hash, sizeof(s->pass_hash));
    s->hasVoted = 0;
    s->next = students_head; students_head = s;
    save_students();
    printf("Registration successful. You can now login to vote if election is active.\n");
}

void add_candidate() {
    char name[MAX_NAME], dept[64];
    printf("\n=== Add Candidate ===\n");
    printf("Name: "); fgets(name, sizeof(name), stdin); trim_newline(name);
    printf("Department: "); fgets(dept, sizeof(dept), stdin); trim_newline(dept);
    Candidate *c = malloc(sizeof(Candidate));
    c->candidate_id = next_candidate_id++;
    strncpy(c->name, name, MAX_NAME);
    strncpy(c->department, dept, sizeof(c->department));
    c->votes = 0;
    c->next = candidates_head; candidates_head = c;
    save_candidates();
    printf("Candidate added with ID %d\n", c->candidate_id);
}

void remove_candidate() {
    int id; printf("Enter candidate ID to remove: "); scanf("%d", &id); getchar();
    Candidate *p = candidates_head, *prev = NULL;
    while (p) {
        if (p->candidate_id == id) {
            if (prev) prev->next = p->next; else candidates_head = p->next;
            free(p); save_candidates(); printf("Candidate removed.\n"); return;
        }
        prev = p; p = p->next;
    }
    printf("Candidate not found.\n");
}

void view_candidates() {
    printf("\n--- Candidate List ---\n");
    Candidate *p = candidates_head;
    if (!p) { printf("No candidates found.\n"); return; }
    while (p) {
        printf("ID: %d | Name: %s | Dept: %s | Votes: %d\n", p->candidate_id, p->name, p->department, p->votes);
        p = p->next;
    }
}

void export_results_csv() {
    FILE *f = fopen(RESULTS_FILE, "w");
    if (!f) { perror("Export failed"); return; }
    fprintf(f, "candidate_id,name,department,votes\n");
    Candidate *p = candidates_head;
    while (p) {
        fprintf(f, "%d,%s,%s,%d\n", p->candidate_id, p->name, p->department, p->votes);
        p = p->next;
    }
    fclose(f);
    printf("Results exported to %s\n", RESULTS_FILE);
}

void declare_winner() {
    if (!candidates_head) { printf("No candidates.\n"); return; }
    Candidate *p = candidates_head;
    Candidate *best = p;
    while (p) { if (p->votes > best->votes) best = p; p = p->next; }
    printf("\nWinner: %s (ID %d) with %d votes.\n", best->name, best->candidate_id, best->votes);
    printf("\n ====THANK YOU =======\n");
}

void admin_menu() {
    const char ADMIN_USER[] = "nub";//admin id
    const char ADMIN_PASS[] = "nub";//pass
    char user[64], pass[64];
    printf("          \n=== Admin Login ===\n");
    printf("          Username: "); fgets(user, sizeof(user), stdin); trim_newline(user);
    printf("          Password: "); fgets(pass, sizeof(pass), stdin); trim_newline(pass);
    if (strcmp(user, ADMIN_USER) != 0 || strcmp(pass, ADMIN_PASS) != 0) {
        printf("Invalid admin credentials.\n"); return;
    }
    int choice;
    while (1) {
        printf("     \n--- Admin Panel ---\n");
        printf("       1. Add Candidate\n");
        printf("       2. Remove Candidate\n");
        printf("       3. View Candidates\n");
        printf("       4. Start Election\n");
        printf("       5. Stop Election\n");
        printf("       6. View Total Votes\n");
        printf("       7. Declare Winner\n");
        printf("       8. Export Results CSV\n");
        printf("       9. Logout\n");
        printf("       \nChoose: "); scanf("%d", &choice); getchar();
        switch (choice) {
            case 1: add_candidate(); break;
            case 2: remove_candidate(); break;
            case 3: view_candidates(); break;
            case 4: election_active = 1; printf("Election started (open).\n"); break;
            case 5: election_active = 0; printf("Election stopped (closed).\n"); break;
            case 6: {
                Candidate *p = candidates_head; int total=0; while (p) { total += p->votes; p=p->next; }
                printf("Total votes cast: %d\n", total);
                break;
            }
            case 7: declare_winner(); break;
            case 8: export_results_csv(); break;
            case 9: return;
            default: printf("Invalid.\n");
        }
    }
}

void cast_vote_student(Student *s) {
    if (!election_active) { printf("Election is not active now.\n"); return; }
    if (s->hasVoted) { printf("You have already voted.\n"); return; }
    view_candidates();
    int cid; printf("Enter candidate ID to vote: "); scanf("%d", &cid); getchar();
    Candidate *c = find_candidate_by_id(cid);
    if (!c) { printf("Invalid candidate ID.\n"); return; }

    c->votes += 1;
    s->hasVoted = 1;

    VoteRecord *rec = malloc(sizeof(VoteRecord));
    strncpy(rec->student_id, s->student_id, MAX_ID);
    rec->candidate_id = cid;
    current_time_str(rec->timestamp, sizeof(rec->timestamp));
    rec->next = votes_head; votes_head = rec;
    save_vote_record(rec);
    save_candidates();
    save_students();
    printf("Vote cast successfully for %s.\n", c->name);
}

void student_login() {
    char sid[MAX_ID], pass[MAX_PASS];
    printf("\n=== Student Login ===\n");
    printf("Student ID: "); fgets(sid, sizeof(sid), stdin); trim_newline(sid);
    printf("Password: "); fgets(pass, sizeof(pass), stdin); trim_newline(pass);
    Student *s = find_student_by_id(sid);
    if (!s) { printf("Student not registered. Register first.\n"); return; }
    if (!verify_password(s, pass)) { printf("Wrong password.\n"); return; }
    int ch;
    while (1) {
        printf("\n--- Voter Panel (%s) ---\n", s->student_id);
        printf("       1. View Candidates\n");
        printf("       2. Cast Vote\n");
        printf("       3. Logout\n");
        printf("       Choose: ");
        scanf("%d", &ch); getchar();
        if (ch == 1) view_candidates();
        else if (ch == 2) cast_vote_student(s);
        else if (ch == 3) break;
        else printf("Invalid.\n");
    }
}

// -
int main_menu() {
    load_students(); load_candidates(); load_votes();
    int choice;
    while (1) {
        printf("\n                     ===Online Voting System (Prototype) ===\n");
        printf("                               1. Admin Login\n");
        printf("                               2. Student Registration\n");
        printf("                               3. Student Login\n");
        printf("                               4. Exit\n");
        printf("\nChoose: "); scanf("%d", &choice); getchar();
        switch (choice) {
            case 1: admin_menu(); break;
            case 2: register_student(); break;
            case 3: student_login(); break;
            case 4: printf("Exiting...\n"); return 0;
            default: printf("Invalid choice.\n");
        }
    }
}

int main() {
printf("\n\n\n");
    printf("\t                              - - - - - -WELCOME TO ONLINE VOTING SYSTEM - - - - - -\n\n");
    printf("\t                              - - - - - -Northern University Bangladesh - - - - - -\n\n");
    printf("\t                              _ _ _ ****************************************** _ _ _ \n\n\n\n\n");
    return main_menu();
}
