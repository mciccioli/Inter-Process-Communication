1. Open your terminal
2. Position yourself in this folder ( in the terminal )
3. run "make all"
4. run the view and the app processes generated like this:
    app:
        ./app <list of .cnf files>
    view:
        if running alongside the app process:
            ./vista
        if running from another terminal:
            ./vista <shm_name>

shm_name can be changed in proceso_app_final. It should be a 4 characters name. If a longer name is written, it shouldnt
work