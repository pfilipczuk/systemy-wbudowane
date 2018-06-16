#r "SendGrid"
using System;
using SendGrid.Helpers.Mail;

public static Mail Run(TraceWriter log, string eventHubMessage)
{
    var date = DateTime.Now;
    Mail message = new Mail{};
    Content content = new Content
    {
        Type = "text/plain",
        Value = $"{date} - nastąpiło nieautoryzowane otwarcie drzwi."
    };
    message.AddContent(content);
    return message;
}