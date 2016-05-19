(setq undo-limit 20000000)
(setq undo-strong-limit 40000000)
(setq-default c-basic-offset 8
	      tab-width 8
	      indent-tabs-mode t)

(setq c-default-style "linux")
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

(global-set-key (kbd "C-o") 'open-next-line)

(defun open-previous-line (arg)
  "Open a new line before the current one. See also `newline-and-indent'."
  (interactive "p")
  (beginning-of-line)
  (open-line arg)
  (when newline-and-indent
    (indent-according-to-mode)))

(global-set-key (kbd "M-o") 'open-previous-line)

;; Autoindent open-*-lines
(defvar newline-and-indent t
  "Modify the behavior of the open-*-line functions to cause them to autoindent.")

(global-hl-line-mode 1)
(set-face-background 'hl-line "lime green")

(if window-system
    (tool-bar-mode 0))

;(require 'ido)
;(ido-mode t)

;; TODO: M-< to start of file
;; TODO: M-> to end of file
;; TODO: Custom font-lock mode for C
;;       - Comments
;;       - Strings?
;;       - Preprocessor
;;       - Faded curly braces?
;; TODO: indentation of `case' items under `switch'

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
  (interactive)
  (menu-bar-mode -1)
  (maximize-frame)
  (set-foreground-color "burlywood3")
  (set-background-color "#161616")
  (set-cursor-color "#40FF40"))

(add-hook 'window-setup-hook 'post-load-stuff t)
