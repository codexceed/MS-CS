import smtplib
import socket


def send(msg, server='smtp-mail.outlook.com', port='587'):
    # contain following in try-except in case of momentary network errors
    try:
        # initialise connection to email server, the default is Outlook
        smtp = smtplib.SMTP(server, port)
        # this is the 'Extended Hello' command, essentially greeting our SMTP or ESMTP server
        smtp.ehlo()
        # this is the 'Start Transport Layer Security' command, tells the server we will
        # be communicating with TLS encryption
        smtp.starttls()

        # read email and password from file
        with open('/Users/perfe/.ssh/email.txt', 'r') as fp:
            email = fp.read()
        with open('/Users/perfe/.ssh/passwd.txt', 'r') as fp:
            pwd = fp.read()

        # login to outlook server
        smtp.login(email, pwd)
        # send notification to self
        smtp.sendmail(email, email, msg.as_string())
        # disconnect from the server
        smtp.quit()
    except socket.gaierror:
        print("Network connection error, email not sent.")