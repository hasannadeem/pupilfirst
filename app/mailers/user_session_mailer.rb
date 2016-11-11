# Mails sent out to startups, as a whole.
class UserSessionMailer < ApplicationMailer
  def send_login_token(user, url, shared_device)
    @user = user
    @referer = url
    @shared_device = shared_device
    mail(to: @user.email, subject: 'Log in to SV.CO')
  end
end
