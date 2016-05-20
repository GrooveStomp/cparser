(setq undo-limit 20000000)
(setq undo-strong-limit 40000000)
(setq-default c-basic-offset 8
	      tab-width 8
	      indent-tabs-mode t)

(setq c-default-style "linux")
(add-hook 'before-save-hook 'delete-trailing-whitespace)
(add-hook 'c-mode-common-hook
	  (lambda ()
	    ;; Make `case' statements in `switch' blocks indent normally.
	    (c-set-offset 'case-label '+)))

(defun open-next-line (arg)
  "Move to the next line and open a new line there. See also `newline-and-indent'."
  (interactive "p")
  (end-of-line)
  (open-line arg)
  (next-line 1)
  (when newline-and-indent
    (indent-according-to-mode)))

(defun open-previous-line (arg)
  "Open a new line before the current one. See also `newline-and-indent'."
  (interactive "p")
  (beginning-of-line)
  (open-line arg)
  (when newline-and-indent
    (indent-according-to-mode)))

;; Delete all whitespace from point to next word.
(defun delete-horizontal-space-forward ()
  "*Delete all spaces and tabs after point."
  (interactive "*")
  (delete-region (point) (progn (skip-chars-forward " \t") (point))))

(global-set-key (kbd "C-o")     'open-next-line)
(global-set-key (kbd "M-o")     'open-previous-line)
(global-set-key (kbd "RET")     'newline-and-indent)
(global-set-key (kbd "C-x C-o") 'dirtree-show)
(global-set-key (kbd "M-\\")    'delete-horizontal-space-forward)
(global-set-key (kbd "C-c C-w") 'copy-word)
(global-set-key (kbd "C-c C-l") 'copy-line)
(global-set-key (kbd "C-c C-p") 'copy-paragraph)
(global-set-key (kbd "M-n")     'forward-paragraph)
(global-set-key (kbd "M-p")     'backward-paragraph)

;; Autoindent open-*-lines
(defvar newline-and-indent t
  "Modify the behavior of the open-*-line functions to cause them to autoindent.")

(if window-system
    (tool-bar-mode 0))

;; TODO: Map right-alt (altgr?) on Dell Mini 10 to Meta.
;;       ^ Then: 'M-<', 'M->' will work.
;;
;; TODO: Custom font-lock mode for C
;;       - Comments
;;       - Strings?
;;       - Preprocessor
;;       - Faded curly braces?

;; Bright red TODOs
(setq fixme-modes '(c++-mode c-mode emacs-lisp-mode))
(make-face 'font-lock-fixme-face)
(make-face 'font-lock-study-face)
(make-face 'font-lock-important-face)
(make-face 'font-lock-note-face)
(mapc (lambda (mode)
	(font-lock-add-keywords
	 mode
	 '(("\\<\\(TODO\\)" 1 'font-lock-fixme-face t)
	   ("\\<\\(STUDY\\)" 1 'font-lock-study-face t)
	   ("\\<\\(IMPORTANT\\)" 1 'font-lock-important-face t)
	   ("\\<\\(NOTE\\)" 1 'font-lock-note-face t))))
      fixme-modes)
(modify-face 'font-lock-fixme-face "Red" nil nil t nil t nil nil)
(modify-face 'font-lock-study-face "Yellow" nil nil t nil t nil nil)
(modify-face 'font-lock-important-face "Yellow" nil nil t nil t nil nil)
(modify-face 'font-lock-note-face "Dark Green" nil nil t nil t nil nil)

(defun post-load-stuff ()
  (setq aaron-fg-color "#cdba96"
	aaron-bg-color "#2e2e2e"
	aaron-hl-color "#3f3f3f"
	)

  (interactive)
  (menu-bar-mode -1)
  (windmove-default-keybindings)
  (set-default 'truncate-lines t)
  (set-foreground-color aaron-fg-color)
  (set-background-color aaron-bg-color)
  (set-cursor-color "#cd6839")

  ;; Set the highlight colors for a selected region.
  (set-face-attribute 'region nil :background "#bd5829")
  (set-face-attribute 'region nil :foreground aaron-bg-color)
  (global-hl-line-mode 1)
  (set-face-background 'hl-line aaron-hl-color)
  (set-face-foreground 'hl-line aaron-fg-color)
  )

(add-hook 'window-setup-hook 'post-load-stuff t)
